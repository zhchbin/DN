// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "proto/echo_example.pb.h"
#include "rpc/rpc_socket_server.h"
#include "rpc/service_manager.h"

class EchoService : public echo::EchoService {
 public:
  void Echo(google::protobuf::RpcController* /*controller*/,
            const echo::EchoRequest* request,
            echo::EchoResponse* response,
            ::google::protobuf::Closure* done) override {
    LOG(INFO) << "Received RPC request: " << request->message();
    response->set_response(request->message());
    done->Run();
  }
};

int main() {
  base::AtExitManager at_exit_manager;
  base::MessageLoopForIO message_loop;
  base::RunLoop run_loop;
  rpc::ServiceManager::GetInstance()->RegisterService(new EchoService());
  rpc::RpcSocketServer server("127.0.0.1", 8909);
  run_loop.Run();

  return 0;
}
