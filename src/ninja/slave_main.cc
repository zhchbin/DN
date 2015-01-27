// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "ninja/slave_main.h"

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_util.h"
#include "proto/rpc_message.pb.h"
#include "rpc/rpc_connection.h"
#include "rpc/rpc_socket_client.h"
#include "thread/ninja_thread.h"

namespace {

static const std::string kWhitelistCommands[] = {
  "g++", "c++", "ninja"
};

void QuitMainThreadHelper() {
  DCHECK(NinjaThread::CurrentlyOn(NinjaThread::MAIN));
  NinjaThread::PostTask(
      NinjaThread::MAIN,
      FROM_HERE,
      base::MessageLoop::current()->QuitClosure());
}

bool IsWhitelistCommand(const std::string& command) {
  for (size_t i = 0; i < arraysize(kWhitelistCommands); ++i) {
    if (StartsWithASCII(command, kWhitelistCommands[i], false))
      return true;
  }

  return false;
}

}  // namespace

namespace ninja {

SlaveMain::SlaveMain(const std::string& master_ip, uint16 port)
    : master_ip_(master_ip),
      port_(port),
      command_runner_(new SlaveCommandRunner()) {
}

SlaveMain::~SlaveMain() {
}

void SlaveMain::Init() {
  rpc::ServiceManager::GetInstance()->RegisterService(this);
}

void SlaveMain::InitAsync() {
  rpc_socket_client_.reset(new rpc::RpcSocketClient(master_ip_, port_));
  rpc_socket_client_->Connect();
}

void SlaveMain::CleanUp() {
  rpc::ServiceManager::GetInstance()->UnregisterService(this);
  command_runner_->CleanUp();
  rpc_socket_client_->Disconnect();
  rpc_socket_client_.reset();
}

void SlaveMain::RunCommand(::google::protobuf::RpcController* /* controller */,
                           const ::slave::RunCommandRequest* request,
                           ::slave::RunCommandResponse* response,
                           ::google::protobuf::Closure* done) {
  DCHECK(NinjaThread::CurrentlyOn(NinjaThread::RPC));
  if (!IsWhitelistCommand(request->command())) {
    response->set_status(slave::RunCommandResponse::kExitFailure);
    response->set_output("This command is NOT ALLOWED to run.");
    done->Run();
  }

  NinjaThread::PostTask(
      NinjaThread::MAIN,
      FROM_HERE,
      base::Bind(&SlaveCommandRunner::AppendCommand,
                 command_runner_,
                 request->command()));
}

void SlaveMain::Finish(::google::protobuf::RpcController* /*controller*/,
                       const ::slave::FinishRequest* /* request */,
                       ::slave::FinishResponse* /* response */,
                       ::google::protobuf::Closure* done) {
  DCHECK(NinjaThread::CurrentlyOn(NinjaThread::RPC));
  if (done)
    done->Run();

  NinjaThread::PostTask(
      NinjaThread::MAIN,
      FROM_HERE,
      base::Bind(&QuitMainThreadHelper));
}

}  // namespace ninja
