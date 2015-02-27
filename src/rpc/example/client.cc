// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "net/net_errors.h"
#include "proto/echo_example.pb.h"
#include "rpc/rpc_socket_client.h"
#include "rpc/service_manager.h"

void EchoDone(echo::EchoResponse* res) {
  scoped_ptr<echo::EchoResponse> response(res);
  LOG(INFO) << "Echo from server: " << response->response();

  base::MessageLoop::current()->QuitNow();
}

void EchoRequest(rpc::RpcSocketClient* client, int rv) {
  if (rv != net::OK)
    return;

  echo::EchoService::Stub stub(client->connection());
  echo::EchoRequest request;
  echo::EchoResponse* response = new echo::EchoResponse;
  request.set_message("hello");
  stub.Echo(NULL, &request, response,
            google::protobuf::NewCallback(EchoDone, response));
}

int main() {
  base::AtExitManager at_exit_manager;
  base::MessageLoopForIO message_loop;
  base::RunLoop run_loop;
  rpc::RpcSocketClient client("127.0.0.1", 8909);
  client.Connect(base::Bind(EchoRequest, &client));
  run_loop.Run();

  return 0;
}
