// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "ninja/slave_main.h"

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "proto/echo.pb.h"
#include "proto/rpc_message.pb.h"
#include "rpc/rpc_connection.h"
#include "rpc/rpc_socket_client.h"
#include "thread/ninja_thread.h"

class EchoServiceImpl2 : public echo::EchoService {
 public:
  virtual void Echo(::google::protobuf::RpcController*,
                       const ::echo::EchoRequest* request,
                       ::echo::EchoResponse* response,
                       ::google::protobuf::Closure* done) {
    LOG(INFO) << "Slave Service is called: " << request->message();
    response->set_response("Echo from slave: " + request->message());
    if (done) {
      done->Run();
    }
  }
};

namespace ninja {

SlaveMain::SlaveMain(const std::string& master_ip, uint16 port)
    : master_ip_(master_ip),
      port_(port) {
}

SlaveMain::~SlaveMain() {
}

void SlaveMain::Init() {
}

void SlaveMain::InitAsync() {
  rpc_socket_client_.reset(new rpc::RpcSocketClient(master_ip_, port_));
  rpc_socket_client_->Connect();
  rpc::ServiceManager::GetInstance()->RegisterService(new EchoServiceImpl2());

  NinjaThread::PostDelayedTask(
      NinjaThread::RPC,
      FROM_HERE,
      base::Bind(&SlaveMain::Echo, base::Unretained(this)),
      base::TimeDelta::FromSeconds(1));
}

void SlaveMain::CleanUp() {
}

void echo_done(echo::EchoResponse* response) {
  LOG(INFO) << response->response();
  delete response;
}

void SlaveMain::Echo() {
  LOG(INFO) << "Echo";
  echo::EchoService::Stub stub(rpc_socket_client_->connection());
  echo::EchoRequest request;
  echo::EchoResponse* response = new echo::EchoResponse;
  request.set_message("hello");
  stub.Echo(NULL, &request, response,
            google::protobuf::NewCallback(echo_done, response));
}

}  // namespace ninja
