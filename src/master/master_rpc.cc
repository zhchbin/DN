// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "master/master_rpc.h"

#include "base/bind.h"
#include "master/master_main_runner.h"
#include "proto/slave_services.pb.h"
#include "rpc/rpc_connection.h"
#include "thread/ninja_thread.h"

namespace {

ExitStatus TransformExitStatus(slave::RunCommandResponse::ExitStatus status) {
  switch (status) {
  case slave::RunCommandResponse::kExitSuccess:
    return ExitSuccess;
  case slave::RunCommandResponse::kExitFailure:
    return ExitFailure;
  case slave::RunCommandResponse::kExitInterrupted:
    return ExitInterrupted;
  default:
    NOTREACHED();
    return ExitFailure;
  }
}

}  // namespace

namespace master {

MasterRPC::MasterRPC(const std::string& bind_ip,
                     uint16 port,
                     MasterMainRunner* master_main_runner)
    : bind_ip_(bind_ip),
      port_(port),
      master_main_runner_(master_main_runner) {
  NinjaThread::SetDelegate(NinjaThread::RPC, this);
}

MasterRPC::~MasterRPC() {
  NinjaThread::SetDelegate(NinjaThread::RPC, NULL);
}

void MasterRPC::Init() {
}

void MasterRPC::InitAsync() {
  rpc_socket_server_.reset(new rpc::RpcSocketServer(bind_ip_, port_));
  rpc_socket_server_->AddObserver(this);
}

void MasterRPC::CleanUp() {
  // Try to notifiy slaves quit event.
  for (Connections::iterator it = connections_.begin();
       it != connections_.end();
       ++it) {
    slave::FinishRequest request;
    slave::FinishResponse response;
    slave::SlaveService::Stub stub(*it);
    stub.Finish(NULL, &request, &response, NULL);
  }
  connections_.clear();

  rpc_socket_server_->RemoveObserver(this);
  rpc_socket_server_.reset(NULL);
}

void MasterRPC::OnConnect(rpc::RpcConnection* connection) {
  connections_.push_back(connection);
  NinjaThread::PostTask(
      NinjaThread::MAIN,
      FROM_HERE,
      base::Bind(&MasterMainRunner::StartBuild, master_main_runner_));
}

void MasterRPC::StartCommandRemotely(const Directories& dirs,
                                     const std::string& rspfile_name,
                                     const std::string& rspfile_content,
                                     const std::string& command,
                                     uint32 edge_id) {
  slave::RunCommandRequest request;
  request.set_command(command);
  request.set_edge_id(edge_id);
  if (!rspfile_name.empty()) {
    request.set_rspfile_name(rspfile_name);
    request.set_rspfile_content(rspfile_content);
  }
  for (Directories::const_iterator it = dirs.begin();
       it != dirs.end();
       ++it) {
    std::string* dir = request.add_dirs();
    dir->assign(*it);
  }

  slave::RunCommandResponse* response = new slave::RunCommandResponse();
  slave::SlaveService::Stub stub(connections_[0]);
  stub.RunCommand(
      NULL,
      &request,
      response,
      google::protobuf::NewCallback(this,
                                    &MasterRPC::OnRemoteCommandDone,
                                    response));
}

void MasterRPC::OnRemoteCommandDone(slave::RunCommandResponse* raw_response) {
  scoped_ptr<slave::RunCommandResponse> response(raw_response);
  NinjaThread::PostTask(
      NinjaThread::MAIN,
      FROM_HERE,
      base::Bind(&MasterMainRunner::OnRemoteCommandDone,
                 master_main_runner_,
                 response->edge_id(),
                 TransformExitStatus(response->status()),
                 response->output()));
}

}  // namespace master
