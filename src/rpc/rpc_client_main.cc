// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "rpc/rpc_client_main.h"

#include "base/bind.h"
#include "base/logging.h"
#include "net/net_errors.h"
#include "rpc/rpc_channel.h"

namespace rpc {

RpcClientMain::RpcClientMain(const std::string& server_ip, uint16 port)
    : server_ip_(server_ip),
      port_(port) {
}

RpcClientMain::~RpcClientMain() {
}

void RpcClientMain::Init() {
}

void RpcClientMain::InitAsync() {
  rpc_channel_.reset(new RpcChannel(server_ip_, port_));
  rpc_channel_->Connect();
}

void RpcClientMain::CleanUp() {
  rpc_channel_.reset(NULL);
}

}  // namespace rpc
