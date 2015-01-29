// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include <string>

#include "base/basictypes.h"
#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "net/net_errors.h"
#include "proto/echo_unittest.pb.h"
#include "rpc/rpc_socket_client.h"
#include "rpc/rpc_socket_server.h"
#include "rpc/service_manager.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;

namespace rpc {

namespace internal {

class TestCompletionCallbackBaseInternal {
 public:
  bool have_result() const { return have_result_; }

 protected:
  TestCompletionCallbackBaseInternal()
      : have_result_(false),
        waiting_for_result_(false) {
  }

  virtual ~TestCompletionCallbackBaseInternal() {}

  void DidSetResult() {
    have_result_ = true;
    if (waiting_for_result_)
      base::MessageLoop::current()->Quit();
  }

  void WaitForResult() {
    DCHECK(!waiting_for_result_);
    while (!have_result_) {
      waiting_for_result_ = true;
      base::MessageLoop::current()->Run();
      waiting_for_result_ = false;
    }
    have_result_ = false;  // Auto-reset for next callback.
  }

  bool have_result_;
  bool waiting_for_result_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestCompletionCallbackBaseInternal);
};

template <typename R>
class TestCompletionCallbackTemplate
    : public TestCompletionCallbackBaseInternal {
 public:
  virtual ~TestCompletionCallbackTemplate() {}

  R WaitForResult() {
    TestCompletionCallbackBaseInternal::WaitForResult();
    return result_;
  }

  R GetResult(R result) {
    if (net::ERR_IO_PENDING != result)
      return result;
    return WaitForResult();
  }

 protected:
  // Override this method to gain control as the callback is running.
  virtual void SetResult(R result) {
    result_ = result;
    DidSetResult();
  }

  TestCompletionCallbackTemplate() : result_(R()) {}
  R result_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestCompletionCallbackTemplate);
};

}  // namespace internal

// Base class overridden by custom implementations of TestCompletionCallback.
typedef internal::TestCompletionCallbackTemplate<int>
    TestCompletionCallbackBase;

class TestCompletionCallback : public TestCompletionCallbackBase {
 public:
  TestCompletionCallback()
      : callback_(base::Bind(&TestCompletionCallback::SetResult,
                  base::Unretained(this))) {
  }

  ~TestCompletionCallback() override {
  }

  const net::CompletionCallback& callback() const { return callback_; }

 private:
  const net::CompletionCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(TestCompletionCallback);
};

static const char kLocalhost[] = "127.0.0.1";
static const uint16 kPort = 20010;
static const uint32 kCallTimes = 10;

class MockEchoService : public echo::EchoService {
 public:
  MOCK_METHOD4(Echo, void(::google::protobuf::RpcController*,
                          const ::echo::EchoRequest*,
                          ::echo::EchoResponse*,
                          ::google::protobuf::Closure* done));
};

void CallService(RpcConnection* connection) {
  for (size_t i = 0; i < kCallTimes; ++i) {
    echo::EchoService::Stub stub(connection);
    echo::EchoRequest request;
    echo::EchoResponse response;
    request.set_message("hello");
    stub.Echo(NULL, &request, &response, NULL);
  }
}

TEST(RpcSocketTest, CallServiceBidirectionally) {
  base::MessageLoopForIO message_loop;
  RpcSocketServer server(kLocalhost, kPort);
  RpcSocketClient client(kLocalhost, kPort);
  MockEchoService* service = new MockEchoService();
  EXPECT_CALL(*service, Echo(_, _, _, _)).Times(2 * kCallTimes);
  ServiceManager::GetInstance()->RegisterService(service);

  TestCompletionCallback connect_callback;
  client.Connect(connect_callback.callback());
  EXPECT_EQ(net::OK, connect_callback.WaitForResult());
  CallService(client.connection());
  CallService(server.FindConnection(0));

  base::MessageLoop::current()->PostTask(
      FROM_HERE,
      base::MessageLoop::current()->QuitClosure());
  message_loop.Run();
}

}  // namespace rpc
