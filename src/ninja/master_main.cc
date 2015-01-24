// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "ninja/master_main.h"

#include "base/big_endian.h"
#include "base/bind.h"
#include "proto/rpc_message.pb.h"
#include "rpc/rpc_socket_server.h"
#include "thread/ninja_thread.h"

namespace ninja {

MasterMain::MasterMain(const std::string& bind_ip, uint16 port)
    : bind_ip_(bind_ip),
      port_(port) {
}

MasterMain::~MasterMain() {
}

void MasterMain::Init() {
}

void MasterMain::InitAsync() {
  rpc_socket_server_.reset(new rpc::RpcSocketServer(bind_ip_, port_));
}

void MasterMain::CleanUp() {
  rpc_socket_server_.reset(NULL);
}

}  // namespace ninja
