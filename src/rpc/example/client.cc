// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "net/net_errors.h"
#include "proto/calculator.pb.h"
#include "rpc/rpc_socket_client.h"
#include "rpc/service_manager.h"

void SumDone(example::SumResponse* res) {
  scoped_ptr<example::SumResponse> response(res);
  LOG(INFO) << "Sum result from server: " << response->sum();

  base::MessageLoop::current()->QuitNow();
}

void Sum(rpc::RpcSocketClient* client, int rv) {
  if (rv != net::OK)
    return;

  example::CalculatorService::Stub stub(client->connection());
  example::SumRequest request;
  request.set_a(19);
  request.set_b(23);
  example::SumResponse* response = new example::SumResponse;
  stub.Sum(NULL, &request, response,
           google::protobuf::NewCallback(SumDone, response));
}

int main() {
  base::AtExitManager at_exit_manager;
  base::MessageLoopForIO message_loop;
  base::RunLoop run_loop;
  rpc::RpcSocketClient client("127.0.0.1", 8909);
  client.Connect(base::Bind(Sum, &client));
  run_loop.Run();

  return 0;
}
