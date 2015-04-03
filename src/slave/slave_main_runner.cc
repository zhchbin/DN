// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "slave/slave_main_runner.h"

#include <set>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/hash.h"
#include "base/md5.h"
#include "base/stl_util.h"
#include "base/threading/thread_restrictions.h"
#include "common/util.h"
#include "ninja/ninja_main.h"
#include "proto/slave_services.pb.h"
#include "slave/slave_file_thread.h"
#include "slave/slave_rpc.h"
#include "thread/ninja_thread.h"

namespace {

slave::RunCommandResponse::ExitStatus TransformExitStatus(ExitStatus status) {
  switch (status) {
  case ExitSuccess:
    return slave::RunCommandResponse::kExitSuccess;
  case ExitFailure:
    return slave::RunCommandResponse::kExitFailure;
  case ExitInterrupted:
    return slave::RunCommandResponse::kExitInterrupted;
  default:
    NOTREACHED();
    return slave::RunCommandResponse::kExitFailure;
  }
}

}  // namespace

namespace slave {

SlaveMainRunner::SlaveMainRunner(const std::string& master, uint16 port)
    : master_(master),
      port_(port),
      command_executor_(new common::CommandExecutor()) {
  command_executor_->AddObserver(this);
}

SlaveMainRunner::~SlaveMainRunner() {
  command_executor_->RemoveObserver(this);
}

void SlaveMainRunner::OnCommandStarted(const std::string& command) {
  LOG(INFO) << command << " Started";
}

void SlaveMainRunner::OnCommandFinished(const std::string& command,
                                        const CommandRunner::Result* result) {
  LOG(INFO) << command << " Finished";
  uint32 command_hash = base::Hash(command);
  RunCommandContextMap::iterator it =
      run_command_context_map_.find(command_hash);
  DCHECK(it != run_command_context_map_.end());
  it->second.response->set_output(result->output);
  it->second.response->set_status(TransformExitStatus(result->status));
  NinjaThread::PostBlockingPoolTask(
      FROM_HERE,
      base::Bind(&SlaveMainRunner::MD5OutputsOnBlockingPool, this, it->second));
  run_command_context_map_.erase(it);
}

bool SlaveMainRunner::PostCreateThreads() {
  slave_rpc_.reset(new SlaveRPC(master_, port_, this));
  slave_file_thread_.reset(new SlaveFileThread());

  std::set<Edge*> edges;
  ninja_main()->GetAllEdges(&edges);
  for (std::set<Edge*>::iterator it = edges.begin(); it != edges.end(); ++it) {
    uint32 hash = common::HashEdge(*it);
    CHECK(ninja_command_hash_map_.find(hash) == ninja_command_hash_map_.end());
    ninja_command_hash_map_[hash] = (*it)->EvaluateCommand();
  }

  return true;
}

void SlaveMainRunner::RunCommand(const RunCommandRequest* request,
                                 RunCommandResponse* response,
                                 ::google::protobuf::Closure* done) {
  uint32 edge_id = request->edge_id();
  response->set_edge_id(edge_id);

  if (!ContainsKey(ninja_command_hash_map_, edge_id)) {
    response->set_status(RunCommandResponse::kExitFailure);
    response->set_output("This command is NOT ALLOWED to run.");
    NinjaThread::PostTask(
        NinjaThread::RPC, FROM_HERE,
        base::Bind(&SlaveRPC::OnRunCommandDone,
                   base::Unretained(slave_rpc_.get()),
                   done));
    return;
  }
  if (!CreateDirsAndResponseFile(request)) {
    response->set_status(RunCommandResponse::kExitFailure);
    response->set_output("Create directories or response file failed.");
    NinjaThread::PostTask(
        NinjaThread::RPC, FROM_HERE,
        base::Bind(&SlaveRPC::OnRunCommandDone,
                   base::Unretained(slave_rpc_.get()),
                   done));
    return;
  }

  std::string& command = ninja_command_hash_map_[edge_id];
  run_command_context_map_[base::Hash(command)] = {request, response, done};
  command_executor_->RunCommand(command);
}

void SlaveMainRunner::Wait() {
}

bool SlaveMainRunner::CreateDirsAndResponseFile(
    const RunCommandRequest* request) {
  base::ThreadRestrictions::AssertIOAllowed();
  for (int i = 0; i < request->output_paths_size(); ++i) {
    base::FilePath dir =
        base::FilePath::FromUTF8Unsafe(request->output_paths(i));
    if (!base::CreateDirectory(dir.DirName()))
      return false;
  }
  if (request->has_rspfile_name()) {
    if (!ninja_main()->disk_interface()->WriteFile(request->rspfile_name(),
                                                   request->rspfile_content()))
      return false;
  }

  return true;
}

void SlaveMainRunner::MD5OutputsOnBlockingPool(
    const RunCommandContext& context) {
  if (context.response->status() == slave::RunCommandResponse::kExitSuccess) {
    for (int i = 0; i < context.request->output_paths_size(); ++i) {
      base::FilePath filename =
          base::FilePath::FromUTF8Unsafe(context.request->output_paths(i));
      std::string md5 = "";
      if (base::PathExists(filename)) {
        md5 = common::GetMd5Digest(filename);
        CHECK(!md5.empty());
      }

      context.response->add_md5()->assign(md5);
    }
  }

  NinjaThread::PostTask(
      NinjaThread::RPC, FROM_HERE,
      base::Bind(&SlaveRPC::OnRunCommandDone,
                 base::Unretained(slave_rpc_.get()),
                 context.done));
}

}  // namespace slave
