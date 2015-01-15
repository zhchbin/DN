// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "rpc/rpc_server_main.h"

#include "base/logging.h"
#include "net/net_errors.h"
#include "net/tcp_server_socket.h"
#include "rpc/rpc_server.h"

namespace {
const int kBackLog = 10;
}  // namespace

namespace rpc {

RpcServerMain::RpcServerMain(const std::string& bind_ip, uint16 port)
    : bind_ip_(bind_ip),
      port_(port) {
}

RpcServerMain::~RpcServerMain() {
}

void RpcServerMain::Init() {
}

void RpcServerMain::InitAsync() {
  scoped_ptr<net::TCPServerSocket> tcp_server_socket;
  tcp_server_socket.reset(new net::TCPServerSocket());
  int result =
      tcp_server_socket->ListenWithAddressAndPort(bind_ip_, port_, kBackLog);
  CHECK(result == net::OK) << "Setup TCP server error.";
  rpc_server_.reset(new rpc::RpcServer(tcp_server_socket.Pass()));
  LOG(INFO) << "RPC Server is listening: " << bind_ip_ << ":" << port_;

  RegisterServices();
}

void RpcServerMain::CleanUp() {
  rpc_server_.reset(NULL);
}

void RpcServerMain::RegisterServices() {
  // Register rpc service here.
}

}  // namespace rpc
