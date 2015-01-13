// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "rpc/rpc_thread_delegate.h"

#include "base/logging.h"
#include "net/net_errors.h"
#include "net/tcp_server_socket.h"
#include "rpc/rpc_server.h"

namespace rpc {

RpcThreadDelegate::RpcThreadDelegate() {
}

RpcThreadDelegate::~RpcThreadDelegate() {  
}

void RpcThreadDelegate::Init() {
}

void RpcThreadDelegate::InitAsync() {
  scoped_ptr<net::TCPServerSocket> tcp_server_socket;
  tcp_server_socket.reset(new net::TCPServerSocket());
  int result =
      tcp_server_socket->ListenWithAddressAndPort("127.0.0.1", 9333, 1024);
  CHECK(result == net::OK) << "Setup TCP server error.";
  rpc_server_.reset(new rpc::RpcServer(tcp_server_socket.Pass()));
}

void RpcThreadDelegate::CleanUp() {
  rpc_server_.reset(NULL);
}

}  // namespace rpc
