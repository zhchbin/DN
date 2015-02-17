// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "master/master_main_runner.h"

#include "base/bind.h"
#include "base/hash.h"
#include "base/sys_info.h"
#include "base/threading/thread_restrictions.h"
#include "common/util.h"
#include "master/curl_helper.h"
#include "master/master_rpc.h"
#include "ninja/dn_builder.h"
#include "ninja/ninja_main.h"
#include "thread/ninja_thread.h"

namespace {

const char kHttp[] = "http://";

}  // namespace

namespace master {

MasterMainRunner::MasterMainRunner(const std::string& bind_ip, uint16 port)
    : bind_ip_(bind_ip),
      port_(port),
      number_of_slave_processors_(0) {
  // |curl_global_init| is not thread-safe, following advice in docs of
  // |curl_easy_init|, we call it manually.
  curl_global_init(CURL_GLOBAL_ALL);
}

MasterMainRunner::~MasterMainRunner() {
  curl_global_cleanup();
}

bool MasterMainRunner::PostCreateThreads() {
  master_rpc_.reset(new MasterRPC(bind_ip_, port_, this));
  return true;
}

void MasterMainRunner::StartBuild() {
  std::string error;
  config_.parallelism = common::GuessParallelism();
  ninja_main()->RunBuild(ninja_main()->state().DefaultNodes(&error), this);
}

bool MasterMainRunner::LocalCanRunMore() {
  size_t subproc_number =
      subprocs_.running_.size() + subprocs_.finished_.size();
  return static_cast<int>(subproc_number) < config_.parallelism;
}

bool MasterMainRunner::RemoteCanRunMore() {
  return static_cast<int>(outstanding_edges_.size()) <=
      number_of_slave_processors_;
}

bool MasterMainRunner::StartCommand(Edge* edge, bool run_in_local) {
  if (run_in_local)
    return StartCommandLocally(edge);
  else
    return StartCommandRemotely(edge);
}

bool MasterMainRunner::StartCommandLocally(Edge* edge) {
  base::ThreadRestrictions::AssertIOAllowed();

  // Create directories necessary for outputs.
  for (vector<Node*>::iterator o = edge->outputs_.begin();
       o != edge->outputs_.end(); ++o) {
    if (!ninja_main()->disk_interface()->MakeDirs((*o)->path()))
      return false;
  }

  // Create response file, if needed
  std::string rspfile = edge->GetUnescapedRspfile();
  if (!rspfile.empty()) {
    std::string content = edge->GetBinding("rspfile_content");
    if (!ninja_main()->disk_interface()->WriteFile(rspfile, content))
      return false;
  }

  std::string command = edge->EvaluateCommand();
  Subprocess* subproc = subprocs_.Add(command, edge->use_console());
  if (!subproc)
    return false;
  subproc_to_edge_.insert(make_pair(subproc, edge));
  return true;
}

bool MasterMainRunner::StartCommandRemotely(Edge* edge) {
  int connection_id = FindMostAvailableSlave();
  if (connection_id == INT_MIN)
    return false;

  MasterRPC::OutputPaths output_paths;
  for (vector<Node*>::iterator o = edge->outputs_.begin();
       o != edge->outputs_.end();
       ++o) {
    output_paths.push_back((*o)->path());
  }

  std::string command = edge->EvaluateCommand();
  uint32 edge_id = base::Hash(command);
  outstanding_edges_[edge_id] = edge;
  NinjaThread::PostTask(
      NinjaThread::RPC,
      FROM_HERE,
      base::Bind(&MasterRPC::StartCommandRemotely,
                 base::Unretained(master_rpc_.get()),
                 connection_id,
                 output_paths,
                 edge->GetUnescapedRspfile(),
                 edge->GetBinding("rspfile_content"),
                 command,
                 edge_id));
  return true;
}

bool MasterMainRunner::WaitForCommand(CommandRunner::Result* result) {
  Subprocess* subproc;
  while ((subproc = subprocs_.NextFinished()) == NULL) {
    bool interrupted = subprocs_.DoWork();
    if (interrupted)
      return false;
  }

  result->status = subproc->Finish();
  result->output = subproc->GetOutput();

  SubprocessToEdgeMap::iterator e = subproc_to_edge_.find(subproc);
  result->edge = e->second;
  subproc_to_edge_.erase(e);

  delete subproc;
  return true;
}

std::vector<Edge*> MasterMainRunner::GetActiveEdges() {
  std::vector<Edge*> edges;
  for (SubprocessToEdgeMap::iterator e = subproc_to_edge_.begin();
       e != subproc_to_edge_.end(); ++e)
    edges.push_back(e->second);
  return edges;
}

void MasterMainRunner::Abort() {
  subprocs_.Clear();
}

bool MasterMainRunner::HasPendingLocalCommands() {
  return !subproc_to_edge_.empty();
}

void MasterMainRunner::OnFetchTargetsDone(CommandRunner::Result result) {
  DCHECK(NinjaThread::CurrentlyOn(NinjaThread::MAIN));
  std::string error;
  if (!ninja_main()->builder()->HasRemoteCommandRunLocally(result.edge))
    ninja_main()->builder()->FinishCommand(&result, &error);
}

void MasterMainRunner::OnRemoteCommandDone(
    int connection_id,
    uint32 edge_id,
    ExitStatus status,
    const std::string& output,
    const std::vector<std::string>& md5s) {
  OutstandingEdgeMap::iterator it = outstanding_edges_.find(edge_id);
  DCHECK(it != outstanding_edges_.end());
  CommandRunner::Result result;
  result.edge = it->second;
  result.status = status;
  result.output = output;  // The output stream of the command.
  outstanding_edges_.erase(it);
  std::string error;
  if (status != ExitSuccess) {
    ninja_main()->builder()->FinishCommand(&result, &error);
    return;
  }

  DCHECK(result.edge->outputs_.size() == md5s.size());
  TargetVector targets;
  for (size_t i = 0; i < result.edge->outputs_.size(); ++i) {
    targets.push_back(
        std::make_pair(result.edge->outputs_[i]->path(), md5s[i]));
  }

  DCHECK(slave_info_id_map_.find(connection_id) != slave_info_id_map_.end());
  // TODO(zhchbin): Remove hard code 8080.
  std::string host = slave_info_id_map_[connection_id].ip + ":" + "8080";
  NinjaThread::PostBlockingPoolTask(
      FROM_HERE,
      base::Bind(&MasterMainRunner::FetchTargetsOnBlockingPool,
                 this,
                 host,
                 targets,
                 result));
}

void MasterMainRunner::OnSlaveSystemInfoAvailable(int connection_id,
                                                  const SlaveInfo& info) {
  if (slave_info_id_map_.find(connection_id) != slave_info_id_map_.end())
    return;
  if (info.operating_system_name != base::SysInfo::OperatingSystemName() ||
      info.operating_system_architecture !=
          base::SysInfo::OperatingSystemArchitecture()) {
    static const string kRejectReason =
        "Different system name or architecture, system info of master: \"" +
        base::SysInfo::OperatingSystemName() + ", " +
        base::SysInfo::OperatingSystemArchitecture() + "\".";

    NinjaThread::PostTask(
        NinjaThread::RPC,
        FROM_HERE,
        base::Bind(&MasterRPC::QuitSlave,
                   base::Unretained(master_rpc_.get()),
                   connection_id,
                   kRejectReason));
    return;
  }

  slave_info_id_map_[connection_id] = info;
  number_of_slave_processors_ += info.number_of_processors;
}

void MasterMainRunner::OnSlaveStatusUpdate(
    int connection_id,
    double load_average,
    int amount_of_running_commands,
    int64 amount_of_available_physical_memory) {
  // Don't update the status until |OnSlaveSystemInfoAvailable| is called.
  SlaveInfoIdMap::iterator it = slave_info_id_map_.find(connection_id);
  if (it == slave_info_id_map_.end())
    return;

  it->second.load_average = load_average;
  it->second.amount_of_running_commands = amount_of_running_commands;
  it->second.amount_of_available_physical_memory =
      amount_of_available_physical_memory;
}

void MasterMainRunner::OnSlaveClose(int connection_id) {
  DCHECK(slave_info_id_map_.find(connection_id) != slave_info_id_map_.end());
  slave_info_id_map_.erase(connection_id);
}

int MasterMainRunner::FindMostAvailableSlave() {
  int connection_id = INT_MIN;
  int max_number_of_available_processors = INT_MIN;
  for (SlaveInfoIdMap::iterator it = slave_info_id_map_.begin();
       it != slave_info_id_map_.end();
       ++it) {
    int tmp =
        it->second.number_of_processors - it->second.amount_of_running_commands;
    if (tmp > max_number_of_available_processors) {
      tmp = max_number_of_available_processors;
      connection_id = it->first;
    }
  }

  return connection_id;
}

void MasterMainRunner::FetchTargetsOnBlockingPool(
    const std::string& host,
    const TargetVector& targets,
    CommandRunner::Result result) {
  bool success = true;
  if (result.success()) {
    master::CurlHelper curl_helper;
    for (size_t i = 0; i < targets.size(); ++i) {
      base::FilePath filename =
          base::FilePath::FromUTF8Unsafe(targets[i].first);
      std::string url = kHttp + host + "/" + targets[i].first;
      success = (curl_helper.Get(url, filename) == targets[i].second);
      if (!success)
        break;
    }
  }

  if (success) {
    NinjaThread::PostTask(
        NinjaThread::MAIN,
        FROM_HERE,
        base::Bind(&MasterMainRunner::OnFetchTargetsDone, this, result));
  }

  // DO NOT call |MasterMainRunner::OnFetchTargetsDone| if curl is failed,
  // since we will try to start unfinished outstanding edges locally.
}

}  // namespace master
