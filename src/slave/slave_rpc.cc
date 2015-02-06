// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "slave/slave_rpc.h"

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_util.h"
#include "proto/rpc_message.pb.h"
#include "rpc/rpc_connection.h"
#include "rpc/rpc_socket_client.h"
#include "rpc/service_manager.h"
#include "slave/slave_main_runner.h"
#include "slave/slave_file_thread.h"
#include "thread/ninja_thread.h"

namespace {

void QuitFileThreadHelper() {
  DCHECK(NinjaThread::CurrentlyOn(NinjaThread::FILE));
  slave::SlaveFileThread::QuitPool();

  NinjaThread::PostTask(
      NinjaThread::FILE,
      FROM_HERE,
      base::MessageLoop::current()->QuitClosure());
}

void QuitMainThreadHelper() {
  DCHECK(NinjaThread::CurrentlyOn(NinjaThread::MAIN));
  NinjaThread::PostTask(
      NinjaThread::MAIN,
      FROM_HERE,
      base::MessageLoop::current()->QuitClosure());
}

}  // namespace

namespace slave {

SlaveRPC::SlaveRPC(const std::string& master_ip,
                   uint16 port,
                   SlaveMainRunner* main_runner)
    : master_ip_(master_ip),
      port_(port),
      slave_main_runner_(main_runner) {
  NinjaThread::SetDelegate(NinjaThread::RPC, this);
}

SlaveRPC::~SlaveRPC() {
  NinjaThread::SetDelegate(NinjaThread::RPC, NULL);
}

void SlaveRPC::Init() {
  rpc::ServiceManager::GetInstance()->RegisterService(this);
}

void SlaveRPC::InitAsync() {
  rpc_socket_client_.reset(new rpc::RpcSocketClient(master_ip_, port_));
  rpc_socket_client_->Connect();
}

void SlaveRPC::CleanUp() {
  rpc::ServiceManager::GetInstance()->UnregisterService(this);
  rpc_socket_client_->Disconnect();
  rpc_socket_client_.reset();
}

void SlaveRPC::RunCommand(google::protobuf::RpcController* /* controller */,
                          const slave::RunCommandRequest* request,
                          slave::RunCommandResponse* response,
                          google::protobuf::Closure* done) {
  DCHECK(NinjaThread::CurrentlyOn(NinjaThread::RPC));
  NinjaThread::PostTask(
      NinjaThread::MAIN, FROM_HERE,
      base::Bind(&SlaveMainRunner::RunCommand, slave_main_runner_,
                 request, response, done));
}

void SlaveRPC::Quit(google::protobuf::RpcController* /*controller*/,
                    const slave::QuitRequest* /* request */,
                    slave::QuitResponse* /* response */,
                    google::protobuf::Closure* done) {
  DCHECK(NinjaThread::CurrentlyOn(NinjaThread::RPC));
  if (done)
    done->Run();

  NinjaThread::PostTask(
      NinjaThread::FILE,
      FROM_HERE,
      base::Bind(&QuitFileThreadHelper));
  NinjaThread::PostTask(
      NinjaThread::MAIN,
      FROM_HERE,
      base::Bind(&QuitMainThreadHelper));
}

void SlaveRPC::OnRunCommandDone(google::protobuf::Closure* done) {
  DCHECK(NinjaThread::CurrentlyOn(NinjaThread::RPC));
  done->Run();
}

}  // namespace slave
