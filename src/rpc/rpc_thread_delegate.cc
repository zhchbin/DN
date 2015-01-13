// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "rpc/rpc_thread_delegate.h"

#include <string>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "net/net_errors.h"
#include "net/tcp_server_socket.h"
#include "rpc/rpc_options.h"
#include "rpc/rpc_server.h"

namespace {
const int kBackLog = 10;
}  // namespace

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

  std::string bind_ip = kDefaultBindIP;
  uint port = kDefaultPort;
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kBindIP))
    bind_ip = command_line->GetSwitchValueASCII(switches::kBindIP);
  if (command_line->HasSwitch(switches::kPort)) {
    std::string port_str = command_line->GetSwitchValueASCII(switches::kPort);
    base::StringToUint(port_str, &port);
  }

  int result =
      tcp_server_socket->ListenWithAddressAndPort(bind_ip, port, kBackLog);
  CHECK(result == net::OK) << "Setup TCP server error.";
  rpc_server_.reset(new rpc::RpcServer(tcp_server_socket.Pass()));
  LOG(INFO) << "RPC Server is listening: " << bind_ip << ":" << port;
}

void RpcThreadDelegate::CleanUp() {
  rpc_server_.reset(NULL);
}

}  // namespace rpc
