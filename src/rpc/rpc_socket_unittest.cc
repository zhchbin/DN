// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include <string>

#include "base/basictypes.h"
#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "rpc/rpc_socket_client.h"
#include "rpc/rpc_socket_server.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "proto/echo.pb.h"

using ::testing::_;

namespace rpc {
namespace {

static const char kLocalhost[] = "127.0.0.1";
static const uint16 kPort = 20010;

class MockEchoService : public echo::EchoService {
 public:
  MOCK_METHOD4(Echo, void(::google::protobuf::RpcController*,
                          const ::echo::EchoRequest*,
                          ::echo::EchoResponse*,
                          ::google::protobuf::Closure* done));
};

void CallService(RpcConnection* connection) {
  echo::EchoService::Stub stub(connection);
  echo::EchoRequest request;
  echo::EchoResponse response;
  request.set_message("hello");
  stub.Echo(NULL, &request, &response, NULL);
}

void CallServerService(RpcSocketClient* client) {
  CallService(client->connection());
}

void CallClientService(RpcSocketServer* server) {
  CallService(server->FindConnection(0));

  base::MessageLoop::current()->PostTask(
      FROM_HERE,
      base::MessageLoop::current()->QuitClosure());
}

TEST(RpcSocketTest, CallServiceBidirectionally) {
  base::MessageLoopForIO message_loop;
  RpcSocketServer server(kLocalhost, kPort);
  RpcSocketClient client(kLocalhost, kPort);
  MockEchoService* service = new MockEchoService();
  EXPECT_CALL(*service, Echo(_, _, _, _)).Times(2);
  ServiceManager::GetInstance()->RegisterService(service);

  client.Connect();

  // Maybe I should expose the connection completed callback, instead of using
  // this tricky way to wait for the connection established.
  base::MessageLoop::current()->PostDelayedTask(
      FROM_HERE,
      base::Bind(CallServerService, &client),
      base::TimeDelta::FromMilliseconds(5));
  base::MessageLoop::current()->PostDelayedTask(
      FROM_HERE,
      base::Bind(CallClientService, &server),
      base::TimeDelta::FromMilliseconds(5));
  message_loop.Run();
}

}  // namespace
}  // namespace rpc
