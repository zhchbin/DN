// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "base/message_loop/message_loop.h"
#include "slave/command_executor.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::Eq;

namespace {

#if defined(OS_WIN)
const char* kSimpleCommand = "cmd /c dir \\";
#else
const char* kSimpleCommand = "ls /";
#endif

}  // namespace

namespace slave {

class MockObserver : public CommandExecutor::Observer {
 public:
  MOCK_METHOD1(OnCommandStarted, void(const std::string&));
  MOCK_METHOD2(OnCommandFinished, void(const std::string&,
                                       const CommandRunner::Result*));
};

TEST(CommandExecutorTest, RumCommands) {
  int times = 5;
  MockObserver observer;
  EXPECT_CALL(observer, OnCommandStarted(Eq(kSimpleCommand))).Times(times);
  EXPECT_CALL(observer, OnCommandFinished(Eq(kSimpleCommand), _)).Times(times);

  CommandExecutor command_executor;
  command_executor.AddObserver(&observer);
  for (int i = 0; i < times; ++i) {
    command_executor.RunCommand(kSimpleCommand);
    command_executor.Wait();
  }
}

}  // namespace slave
