// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "master/master_rpc.h"

#include "base/bind.h"
#include "master/master_main_runner.h"
#include "net/ip_endpoint.h"
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
  timer_.reset(new base::RepeatingTimer<MasterRPC>());
  timer_->Start(FROM_HERE, base::TimeDelta::FromSeconds(2),
                this, &MasterRPC::GetSlavesStatus);
}

void MasterRPC::CleanUp() {
  // Try to notifiy slaves quit event.
  for (ConnectionMap::iterator it = connections_.begin();
       it != connections_.end();
       ++it) {
    static const char kQuitSuccess[] = "Build finished successfully.";
    QuitSlave(it->first, kQuitSuccess);
  }
  connections_.clear();

  rpc_socket_server_->RemoveObserver(this);
  rpc_socket_server_.reset();
  timer_->Stop();
  timer_.reset();
}

void MasterRPC::OnConnect(rpc::RpcConnection* connection) {
  connections_[connection->id()] = connection;

  slave::SystemInfoRequest request;
  slave::SystemInfoResponse* response = new slave::SystemInfoResponse;
  slave::SlaveService::Stub stub(connection);
  stub.SystemInfo(
      NULL, &request, response,
      google::protobuf::NewCallback(this,
                                    &MasterRPC::OnSlaveSystemInfoAvailable,
                                    connection->id(),
                                    response));

  NinjaThread::PostTask(
      NinjaThread::MAIN,
      FROM_HERE,
      base::Bind(&MasterMainRunner::StartBuild, master_main_runner_));
}

void MasterRPC::OnClose(rpc::RpcConnection* connection) {
  connections_.erase(connection->id());
  NinjaThread::PostTask(
      NinjaThread::MAIN,
      FROM_HERE,
      base::Bind(&MasterMainRunner::OnSlaveClose,
                 master_main_runner_,
                 connection->id()));
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
  // TODO(zhchbin): choose one of the connections.
  slave::SlaveService::Stub stub(connections_[0]);
  stub.RunCommand(
      NULL,
      &request,
      response,
      google::protobuf::NewCallback(this,
                                    &MasterRPC::OnRemoteCommandDone,
                                    connections_[0]->id(),
                                    response));
}

void MasterRPC::QuitSlave(int connection_id, const std::string& reason) {
  ConnectionMap::iterator it = connections_.find(connection_id);
  DCHECK(it != connections_.end());
  slave::QuitRequest request;
  if (!reason.empty())
    request.set_reason(reason);

  slave::QuitResponse response;
  slave::SlaveService::Stub stub(it->second);
  stub.Quit(NULL, &request, &response, NULL);
}

void MasterRPC::OnRemoteCommandDone(
    int connection_id,
    slave::RunCommandResponse* raw_response) {
  scoped_ptr<slave::RunCommandResponse> response(raw_response);

  NinjaThread::PostTask(
      NinjaThread::MAIN,
      FROM_HERE,
      base::Bind(&MasterMainRunner::OnRemoteCommandDone,
                 master_main_runner_,
                 connection_id,
                 response->edge_id(),
                 TransformExitStatus(response->status()),
                 response->output()));
}

void MasterRPC::OnSlaveSystemInfoAvailable(
    int connection_id,
    slave::SystemInfoResponse* raw_response) {
  DCHECK(connections_.find(connection_id) != connections_.end());
  scoped_ptr<slave::SystemInfoResponse> response(raw_response);

  SlaveInfo info;
  info.number_of_processors = response->number_of_processors();
  info.amount_of_physical_memory = response->amount_of_physical_memory();
  info.amount_of_virtual_memory = response->amount_of_virtual_memory();
  info.operating_system_name = response->operating_system_name();
  info.operating_system_version = response->operating_system_version();
  info.operating_system_architecture =
      response->operating_system_architecture();

  net::IPEndPoint ip_address;
  connections_[connection_id]->GetPeerAddress(&ip_address);
  info.ip = ip_address.ToStringWithoutPort();

  NinjaThread::PostTask(
      NinjaThread::MAIN,
      FROM_HERE,
      base::Bind(&MasterMainRunner::OnSlaveSystemInfoAvailable,
                 master_main_runner_,
                 connection_id,
                 info));
}

void MasterRPC::GetSlavesStatus() {
  for (ConnectionMap::iterator it = connections_.begin();
       it != connections_.end();
       ++it) {
    slave::StatusRequest request;
    slave::StatusResponse* response = new slave::StatusResponse();
    slave::SlaveService::Stub stub(it->second);
    stub.GetStatus(NULL, &request, response,
        google::protobuf::NewCallback(this,
                                      &MasterRPC::OnSlaveStatusUpdate,
                                      it->first,
                                      response));
  }
}

void MasterRPC::OnSlaveStatusUpdate(int connection_id,
                                    slave::StatusResponse* raw_response) {
  scoped_ptr<slave::StatusResponse> response(raw_response);
  NinjaThread::PostTask(
      NinjaThread::MAIN,
      FROM_HERE,
      base::Bind(&MasterMainRunner::OnSlaveStatusUpdate,
                 master_main_runner_,
                 connection_id,
                 response->load_average(),
                 response->amount_of_running_commands(),
                 response->amount_of_available_physical_memory()));
}

}  // namespace master
