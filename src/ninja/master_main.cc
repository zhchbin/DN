// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "ninja/master_main.h"

#include "base/big_endian.h"
#include "base/bind.h"
#include "proto/echo.pb.h"
#include "proto/rpc_message.pb.h"
#include "rpc/rpc_socket_server.h"
#include "thread/ninja_thread.h"

class EchoServiceImpl : public echo::EchoService {
 public:
  EchoServiceImpl() {}

  virtual void Echo(::google::protobuf::RpcController*,
                       const ::echo::EchoRequest* request,
                       ::echo::EchoResponse* response,
                       ::google::protobuf::Closure* done) {
    LOG(INFO) << "Master Service is called: " << request->message();
    response->set_response("Echo from master: " + request->message());
    if (done) {
      done->Run();
    }
  }
};

namespace ninja {

MasterMain::MasterMain(const std::string& bind_ip, uint16 port)
    : bind_ip_(bind_ip),
      port_(port) {
}

MasterMain::~MasterMain() {
}

void MasterMain::Init() {
  rpc::ServiceManager::GetInstance()->RegisterService(new EchoServiceImpl());
}

void MasterMain::InitAsync() {
  rpc_socket_server_.reset(new rpc::RpcSocketServer(bind_ip_, port_));

  NinjaThread::PostDelayedTask(
      NinjaThread::RPC,
      FROM_HERE,
      base::Bind(&MasterMain::Echo, base::Unretained(this)),
      base::TimeDelta::FromSeconds(3));
}

void MasterMain::CleanUp() {
  rpc_socket_server_.reset(NULL);
}

void echo2_done(echo::EchoResponse* raw_response) {
  scoped_ptr<echo::EchoResponse> response(raw_response);
  LOG(INFO) << response->response();
}

void MasterMain::Echo() {
  LOG(INFO) << "MasterMain Echo";
  echo::EchoService::Stub stub(rpc_socket_server_->FindConnection(0));
  echo::EchoRequest request;
  echo::EchoResponse* response = new echo::EchoResponse;
  request.set_message("MasterMain");
  stub.Echo(NULL, &request, response,
            google::protobuf::NewCallback(echo2_done, response));
}

}  // namespace ninja
