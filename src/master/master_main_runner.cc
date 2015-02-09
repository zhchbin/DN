// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "master/master_main_runner.h"

#include "base/bind.h"
#include "base/hash.h"
#include "base/threading/thread_restrictions.h"
#include "common/util.h"
#include "curl/curl.h"
#include "master/master_rpc.h"
#include "ninja/dn_builder.h"
#include "ninja/ninja_main.h"
#include "thread/ninja_thread.h"

namespace master {

MasterMainRunner::MasterMainRunner(const std::string& bind_ip, uint16 port)
    : bind_ip_(bind_ip),
      port_(port),
      number_of_slave_processors_(0) {
}

MasterMainRunner::~MasterMainRunner() {
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
  return static_cast<int>(outstanding_edges_.size()) <
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
  MasterRPC::Directories dirs;
  for (vector<Node*>::iterator o = edge->outputs_.begin();
       o != edge->outputs_.end(); ++o) {
    dirs.push_back((*o)->path());
  }

  std::string command = edge->EvaluateCommand();
  uint32 edge_id = base::Hash(command);
  outstanding_edges_[edge_id] = edge;
  NinjaThread::PostTask(
      NinjaThread::RPC,
      FROM_HERE,
      base::Bind(&MasterRPC::StartCommandRemotely,
                 base::Unretained(master_rpc_.get()),
                 dirs,
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


size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
  size_t written = fwrite(ptr, size, nmemb, stream);
  return written;
}

void CurlTargetOnFileThread(const std::string& target) {
  DCHECK(NinjaThread::CurrentlyOn(NinjaThread::FILE));

  CURL *curl;
  FILE *fp;
  std::string url = "http://127.0.0.1:8080/" + target;
  curl = curl_easy_init();
  if (curl) {
    fp = fopen(target.c_str(), "wb");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    fclose(fp);
  }
}

void MasterMainRunner::OnCurlTargetDone(CommandRunner::Result result) {
  DCHECK(NinjaThread::CurrentlyOn(NinjaThread::MAIN));
  std::string error;
  if (!ninja_main()->builder()->HasRemoteCommandRunLocally(result.edge)) {
    ninja_main()->builder()->FinishCommand(&result, &error);
  }
}

void MasterMainRunner::OnRemoteCommandDone(uint32 edge_id,
                                           ExitStatus status,
                                           const std::string& output) {
  OutstandingEdgeMap::iterator it = outstanding_edges_.find(edge_id);
  DCHECK(it != outstanding_edges_.end());
  CommandRunner::Result result;
  result.edge = it->second;
  result.status = status;
  result.output = output;
  outstanding_edges_.erase(it);
  std::string error;

  NinjaThread::PostTaskAndReply(
      NinjaThread::FILE,
      FROM_HERE,
      base::Bind(CurlTargetOnFileThread, result.edge->outputs_[0]->path()),
      base::Bind(&MasterMainRunner::OnCurlTargetDone, this, result));
}

void MasterMainRunner::OnSlaveSystemInfoAvailable(int connection_id,
                                                  const SlaveInfo& info) {
  if (slave_info_id_map_.find(connection_id) != slave_info_id_map_.end())
    return;

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

}  // namespace master
