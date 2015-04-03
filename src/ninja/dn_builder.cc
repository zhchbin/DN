// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "ninja/dn_builder.h"

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "master/master_main_runner.h"
#include "third_party/ninja/src/build_log.h"
#include "third_party/ninja/src/depfile_parser.h"
#include "third_party/ninja/src/deps_log.h"
#include "third_party/ninja/src/disk_interface.h"
#include "third_party/ninja/src/state.h"
#include "thread/ninja_thread.h"

#if defined(OS_WIN)
#include "third_party/ninja/src/msvc_helper.h"
#endif

using master::MasterMainRunner;

namespace {

bool EdgeMustStartLocally(Edge* edge) {
  for (std::vector<Node*>::iterator it = edge->inputs_.begin();
       it != edge->inputs_.end();
       ++it) {
    if ((*it)->in_edge() == NULL)
      return false;

    if (!(*it)->in_edge()->inputs_.empty())
      return true;
  }

  return false;
}

}  // namespace

namespace ninja {

DNBuilder::DNBuilder(State* state,
                     const BuildConfig& config,
                     BuildLog* build_log,
                     DepsLog* deps_log,
                     DiskInterface* disk_interface)
    : state_(state),
      config_(config),
      command_runner_(NULL),
      disk_interface_(disk_interface),
      scan_(state, build_log, deps_log, disk_interface),
      weak_factory_(this) {
  status_.reset(new BuildStatus(config));
  command_executor_.AddObserver(this);
}

DNBuilder::~DNBuilder() {
  command_executor_.RemoveObserver(this);
}

Node* DNBuilder::AddTarget(const string& name, string* err) {
  Node* node = state_->LookupNode(name);
  if (!node) {
    *err = "unknown target: '" + name + "'";
    return NULL;
  }
  if (!AddTarget(node, err))
    return NULL;
  return node;
}

bool DNBuilder::AddTarget(Node* node, string* err) {
  if (Edge* in_edge = node->in_edge()) {
    if (!scan_.RecomputeDirty(in_edge, err))
      return false;
    if (in_edge->outputs_ready())
      return true;  // Nothing to do.
  }

  if (!plan_.AddTarget(node, err))
    return false;

  return true;
}

bool DNBuilder::AlreadyUpToDate() const {
  return !plan_.more_to_do();
}

bool DNBuilder::Build(string* err, master::MasterMainRunner* runner) {
  command_runner_ = runner;
  status_->PlanHasTotalEdges(plan_.command_edge_count());

  const std::set<Edge*>& command_edge_set = plan_.command_edge_set();
  scoped_ptr<base::ListValue> commands(new base::ListValue);
  int last_id = 0;
  for (std::set<Edge*>::iterator it = command_edge_set.begin();
       it != command_edge_set.end();
       ++it) {
    (*it)->id_ = last_id++;
    base::DictionaryValue* command_edge = new base::DictionaryValue();
    command_edge->SetInteger("id", (*it)->id_);
    std::string content;
    content = (*it)->rule().name() + " ";
    for (vector<Node*>::iterator out = (*it)->outputs_.begin();
         out != (*it)->outputs_.end(); ++out) {
      content += (*out)->path() + " ";
    }
    command_edge->SetString("content", content);
    commands->Append(command_edge);
  }
  std::string json;
  base::JSONWriter::Write(commands.get(), &json);
  runner->SetWebUIInitialStatus(json);
  start_build_time_ = base::Time::Now();

  InitialalBuild();
  return true;
}

void DNBuilder::InitialalBuild() {
  DCHECK(!plan_.more_to_do());

  std::string error;

  while (command_executor_.CanRunMore()) {
    Edge* edge = plan_.FindWork();
    if (edge == NULL)
      break;

    if (!StartEdge(edge, &error, true))
      break;
  }

  if (!plan_.more_to_do()) {
    BuildFinished();
    return;
  }

  const MasterMainRunner::SlaveInfoIdMap& slaves = command_runner_->GetSlaves();
  for (MasterMainRunner::SlaveInfoIdMap::const_iterator it = slaves.begin();
       it != slaves.end();
       ++it) {
    int edge_counter = 0;
    while (edge_counter <= it->second.number_of_processors) {
      Edge* edge = plan_.FindWork();
      if (edge == NULL)
        break;

      status_->BuildEdgeStarted(edge);
      command_runner_->StartEdgeRemotelly(edge, it->first);
      outstanding_edge_list_.push_back(edge);
      edge_counter++;
    }
  }
}


bool DNBuilder::HasRemoteCommandRunLocally(Edge* edge) {
  DCHECK(NinjaThread::CurrentlyOn(NinjaThread::MAIN));
  for (OutstandingEdgeList::iterator it = outstanding_edge_list_.begin();
       it != outstanding_edge_list_.end();
       ++it) {
    if (*it == edge) {
      return false;
    }
  }

  return true;
}

void DNBuilder::BuildLoop() {
  DCHECK(NinjaThread::CurrentlyOn(NinjaThread::MAIN));
  DCHECK(command_runner_ != NULL);
  if (!plan_.more_to_do()) {
    BuildFinished();
    return;
  }

  std::string error;
  while (command_executor_.CanRunMore()) {
    Edge* edge = plan_.FindWork();
    if (edge == NULL) {
      if (outstanding_edge_list_.empty())
        break;

      edge = outstanding_edge_list_.front();
      outstanding_edge_list_.pop_front();
    }

    if (!StartEdge(edge, &error, true)) {
      LOG(ERROR) << "Can not start edge locally. " << error;
      return;
    }
  }

  if (!plan_.more_to_do()) {
    BuildFinished();
    return;
  }
}

bool DNBuilder::StartEdge(Edge* edge, string* err, bool run_in_local) {
  METRIC_RECORD("StartEdge");
  if (edge->is_phony()) {
    plan_.EdgeFinished(edge);
    return true;
  }
  status_->BuildEdgeStarted(edge);

  // Create directories necessary for outputs.
  for (vector<Node*>::iterator o = edge->outputs_.begin();
       o != edge->outputs_.end(); ++o) {
    base::FilePath dir =
        base::FilePath::FromUTF8Unsafe((*o)->path());
    if (!base::CreateDirectory(dir.DirName()))
      return false;
  }

  // Create response file, if needed
  std::string rspfile = edge->GetUnescapedRspfile();
  if (!rspfile.empty()) {
    std::string content = edge->GetBinding("rspfile_content");
    if (disk_interface_->WriteFile(rspfile, content))
      return false;
  }

  std::string command = edge->EvaluateCommand();
  command_edge_map_[command] = edge;
  command_executor_.RunCommand(edge->EvaluateCommand());
  return true;
}

bool DNBuilder::FinishCommand(CommandRunner::Result* result, string* err) {
  DCHECK(NinjaThread::CurrentlyOn(NinjaThread::MAIN));
  METRIC_RECORD("FinishCommand");
  command_runner_->BuildEdgeFinished(result);

  if (!result->success()) {
    LOG(ERROR) << "subcommand failed: " << result->output;
    BuildFinished();
    return false;
  }

  Edge* edge = result->edge;
  for (OutstandingEdgeList::iterator it = outstanding_edge_list_.begin();
       it != outstanding_edge_list_.end();
       ++it) {
    if (*it == edge) {
      outstanding_edge_list_.erase(it);
      break;
    }
  }

  // First try to extract dependencies from the result, if any.
  // This must happen first as it filters the command output (we want
  // to filter /showIncludes output, even on compile failure) and
  // extraction itself can fail, which makes the command fail from a
  // build perspective.
  vector<Node*> deps_nodes;
  string deps_type = edge->GetBinding("deps");
  const string deps_prefix = edge->GetBinding("msvc_deps_prefix");
  if (!deps_type.empty()) {
    string extract_err;
    if (!ExtractDeps(result, deps_type, deps_prefix, &deps_nodes,
                     &extract_err) &&
        result->success()) {
      if (!result->output.empty())
        result->output.append("\n");
      result->output.append(extract_err);
      result->status = ExitFailure;
    }
  }

  int start_time, end_time;
  status_->BuildEdgeFinished(edge, result->success(), result->output,
                             &start_time, &end_time);

  // The rest of this function only applies to successful commands.
  if (!result->success())
    return true;

  // Restat the edge outputs, if necessary.
  TimeStamp restat_mtime = 0;
  if (edge->GetBindingBool("restat") && !config_.dry_run) {
    bool node_cleaned = false;

    for (vector<Node*>::iterator o = edge->outputs_.begin();
         o != edge->outputs_.end(); ++o) {
      TimeStamp new_mtime = disk_interface_->Stat((*o)->path());
      if ((*o)->mtime() == new_mtime) {
        // The rule command did not change the output.  Propagate the clean
        // state through the build graph.
        // Note that this also applies to nonexistent outputs (mtime == 0).
        plan_.CleanNode(&scan_, *o);
        node_cleaned = true;
      }
    }

    if (node_cleaned) {
      // If any output was cleaned, find the most recent mtime of any
      // (existing) non-order-only input or the depfile.
      for (vector<Node*>::iterator i = edge->inputs_.begin();
           i != edge->inputs_.end() - edge->order_only_deps_; ++i) {
        TimeStamp input_mtime = disk_interface_->Stat((*i)->path());
        if (input_mtime > restat_mtime)
          restat_mtime = input_mtime;
      }

      string depfile = edge->GetUnescapedDepfile();
      if (restat_mtime != 0 && deps_type.empty() && !depfile.empty()) {
        TimeStamp depfile_mtime = disk_interface_->Stat(depfile);
        if (depfile_mtime > restat_mtime)
          restat_mtime = depfile_mtime;
      }

      // The total number of edges in the plan may have changed as a result
      // of a restat.
      status_->PlanHasTotalEdges(plan_.command_edge_count());
    }
  }

  plan_.EdgeFinished(edge);

  // Delete any left over response file.
  string rspfile = edge->GetUnescapedRspfile();
  if (!rspfile.empty())
    disk_interface_->RemoveFile(rspfile);

  if (scan_.build_log()) {
    if (!scan_.build_log()->RecordCommand(edge, start_time, end_time,
                                          restat_mtime)) {
      *err = string("Error writing to build log: ") + strerror(errno);
      return false;
    }
  }

  if (!deps_type.empty() && !config_.dry_run) {
    assert(edge->outputs_.size() == 1 && "should have been rejected by parser");
    Node* out = edge->outputs_[0];
    TimeStamp deps_mtime = disk_interface_->Stat(out->path());
    if (!scan_.deps_log()->RecordDeps(out, deps_mtime, deps_nodes)) {
      *err = string("Error writing to deps log: ") + strerror(errno);
      return false;
    }
  }
  return true;
}

bool DNBuilder::ExtractDeps(CommandRunner::Result* result,
                            const string& deps_type,
                            const string& deps_prefix,
                            vector<Node*>* deps_nodes,
                            string* err) {
#ifdef _WIN32
  if (deps_type == "msvc") {
    CLParser parser;
    result->output = parser.Parse(result->output, deps_prefix);
    for (set<string>::iterator i = parser.includes_.begin();
         i != parser.includes_.end(); ++i) {
      // ~0 is assuming that with MSVC-parsed headers, it's ok to always make
      // all backslashes (as some of the slashes will certainly be backslashes
      // anyway). This could be fixed if necessary with some additional
      // complexity in IncludesNormalize::Relativize.
      deps_nodes->push_back(state_->GetNode(*i, ~0u));
    }
  } else         // NOLINT
#endif
  if (deps_type == "gcc") {
    string depfile = result->edge->GetUnescapedDepfile();
    if (depfile.empty()) {
      *err = string("edge with deps=gcc but no depfile makes no sense");
      return false;
    }

    string content = disk_interface_->ReadFile(depfile, err);
    if (!err->empty())
      return false;
    if (content.empty())
      return true;

    DepfileParser deps;
    if (!deps.Parse(&content, err))
      return false;

    // XXX check depfile matches expected output.
    deps_nodes->reserve(deps.ins_.size());
    for (vector<StringPiece>::iterator i = deps.ins_.begin();
         i != deps.ins_.end(); ++i) {
      unsigned int slash_bits;
      if (!CanonicalizePath(const_cast<char*>(i->str_), &i->len_, &slash_bits,
                            err))
        return false;
      deps_nodes->push_back(state_->GetNode(*i, slash_bits));
    }

    if (disk_interface_->RemoveFile(depfile) < 0) {
      *err = string("deleting depfile: ") + strerror(errno) + string("\n");
      return false;
    }
  } else {
    Fatal("unknown deps type '%s'", deps_type.c_str());
  }

  return true;
}

void DNBuilder::BuildFinished() {
  base::TimeDelta time_between_use = base::Time::Now() - start_build_time_;
  LOG(INFO) << time_between_use.InSecondsF();
  status_->BuildFinished();
  command_runner_->BuildFinished();
}

void DNBuilder::OnCommandStarted(const std::string& command) {
}

void DNBuilder::OnCommandFinished(const std::string& command,
                                  const CommandRunner::Result* result) {
  std::map<std::string, Edge*>::iterator it = command_edge_map_.find(command);
  DCHECK(it != command_edge_map_.end());
  CommandRunner::Result r = *result;
  r.edge = it->second;
  std::string error;
  FinishCommand(&r, &error);

  BuildLoop();
}

void DNBuilder::RequestEdge(int connection_id) {
  if (!plan_.more_to_do())
    return;

  Edge* edge = plan_.FindWork();
  // TODO(zhchbin): |edge| should be able to run remotelly.
  if (edge == NULL)
    return;

  status_->BuildEdgeStarted(edge);
  command_runner_->StartEdgeRemotelly(edge, connection_id);
  outstanding_edge_list_.push_back(edge);
}

}  // namespace ninja
