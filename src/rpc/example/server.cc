// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "proto/calculator.pb.h"
#include "rpc/rpc_socket_server.h"
#include "rpc/service_manager.h"

class CalculatorService : public example::CalculatorService {
 public:
  void Sum(google::protobuf::RpcController* /*controller*/,
           const example::SumRequest* request,
           example::SumResponse* response,
           ::google::protobuf::Closure* done) override {
    response->set_sum(request->a() + request->b());
    done->Run();
  }
};

int main() {
  base::AtExitManager at_exit_manager;
  base::MessageLoopForIO message_loop;
  base::RunLoop run_loop;
  rpc::ServiceManager::GetInstance()->RegisterService(new CalculatorService());
  rpc::RpcSocketServer server("127.0.0.1", 8909);
  run_loop.Run();

  return 0;
}
