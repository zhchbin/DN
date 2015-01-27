// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "ninja/slave_main.h"

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "proto/rpc_message.pb.h"
#include "rpc/rpc_connection.h"
#include "rpc/rpc_socket_client.h"
#include "thread/ninja_thread.h"

namespace ninja {

SlaveMain::SlaveMain(const std::string& master_ip, uint16 port)
    : master_ip_(master_ip),
      port_(port),
      command_runner_(new SlaveCommandRunner()) {
}

SlaveMain::~SlaveMain() {
}

void SlaveMain::Init() {
}

void SlaveMain::InitAsync() {
  rpc_socket_client_.reset(new rpc::RpcSocketClient(master_ip_, port_));
  rpc_socket_client_->Connect();
}

void SlaveMain::CleanUp() {
}


}  // namespace ninja
