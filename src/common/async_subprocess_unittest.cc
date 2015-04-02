// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "common/async_subprocess.h"
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


namespace common {

void ExitCallback(AsyncSubprocess* proc) {
  base::MessageLoop::current()->Quit();
}

TEST(AsyncSubprocessTest, RunCommand) {
  base::MessageLoopForIO message_loop;
  AsyncSubprocess aysnc_subprocess;
  aysnc_subprocess.Start(kSimpleCommand, base::Bind(ExitCallback));
  message_loop.Run();
  EXPECT_EQ(aysnc_subprocess.Finish(), ExitSuccess);
}

static const int kProcessesSize = 10;
static int counter = 0;

void ExitCallbackCount(AsyncSubprocess* proc) {
  counter++;
  if (counter == kProcessesSize)
    base::MessageLoop::current()->Quit();
}

TEST(AsyncSubprocessTest, RunCommandParallelly) {
  base::MessageLoopForIO message_loop;
  counter = 0;
  scoped_ptr<AsyncSubprocess> aysnc_subprocess[kProcessesSize];
  for (int i = 0; i < kProcessesSize; ++i) {
    aysnc_subprocess[i].reset(new AsyncSubprocess());
    aysnc_subprocess[i]->Start(kSimpleCommand, base::Bind(ExitCallbackCount));
  }
  message_loop.Run();
  for (int i = 0; i < kProcessesSize; ++i)
    EXPECT_EQ(aysnc_subprocess[i]->Finish(), ExitSuccess);
}

}  // namespace common
