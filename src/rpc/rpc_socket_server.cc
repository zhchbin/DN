// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "rpc/rpc_socket_server.h"

#include "base/big_endian.h"
#include "base/message_loop/message_loop_proxy.h"
#include "base/stl_util.h"
#include "base/tracked_objects.h"
#include "net/net_errors.h"
#include "net/server_socket.h"
#include "net/stream_socket.h"
#include "net/tcp_server_socket.h"

namespace {
const int kBackLog = 10;
}  // namespace

namespace rpc {

RpcSocketServer::RpcSocketServer(const std::string& bind_ip, uint16 port)
    : last_id_(0),
      bind_ip_(bind_ip),
      port_(port),
      weak_ptr_factory_(this) {
  Init();
}

RpcSocketServer::~RpcSocketServer() {
  STLDeleteContainerPairSecondPointers(
      id_to_connection_.begin(), id_to_connection_.end());
}

void RpcSocketServer::AddObserver(Observer *obs) {
  observer_list_.AddObserver(obs);
}

void RpcSocketServer::RemoveObserver(Observer *obs) {
  observer_list_.RemoveObserver(obs);
}

void RpcSocketServer::Init() {
  server_socket_.reset(new net::TCPServerSocket());
  int result =
      server_socket_->ListenWithAddressAndPort(bind_ip_, port_, kBackLog);
  CHECK(result == net::OK) << "Setup TCP server error.";
  LOG(INFO) << "RPC Socket Server is listening: " << bind_ip_ << ":" << port_;

  // Start accepting connections in next run loop in case when delegate is not
  // ready to get callbacks.
  base::MessageLoopProxy::current()->PostTask(
      FROM_HERE,
      base::Bind(&RpcSocketServer::DoAcceptLoop,
                 weak_ptr_factory_.GetWeakPtr()));
}

void RpcSocketServer::DoAcceptLoop() {
  int rv;
  do {
    rv = server_socket_->Accept(&accepted_socket_,
                                base::Bind(&RpcSocketServer::OnAcceptCompleted,
                                           weak_ptr_factory_.GetWeakPtr()));
    if (rv == net::ERR_IO_PENDING)
      return;
    rv = HandleAcceptResult(rv);
  } while (rv == net::OK);
}

void RpcSocketServer::OnAcceptCompleted(int rv) {
  if (HandleAcceptResult(rv) == net::OK)
    DoAcceptLoop();
}

int RpcSocketServer::HandleAcceptResult(int rv) {
  if (rv < 0) {
    LOG(ERROR) << "Accept error: rv=" << rv;
    return rv;
  }

  RpcConnection* connection =
      new RpcConnection(last_id_++, accepted_socket_.Pass(), this);
  id_to_connection_[connection->id()] = connection;
  connection->DoReadLoop();
  FOR_EACH_OBSERVER(Observer, observer_list_, OnConnect(connection));
  return net::OK;
}

RpcConnection* RpcSocketServer::FindConnection(int connection_id) {
  IdToConnectionMap::iterator it = id_to_connection_.find(connection_id);
  if (it == id_to_connection_.end())
    return NULL;

  return it->second;
}

void RpcSocketServer::OnClose(RpcConnection* connection) {
  DCHECK(id_to_connection_.find(connection->id()) != id_to_connection_.end());
  FOR_EACH_OBSERVER(Observer, observer_list_, OnClose(connection));
  id_to_connection_.erase(connection->id());
  base::MessageLoopProxy::current()->DeleteSoon(FROM_HERE, connection);
}

}  // namespace rpc
