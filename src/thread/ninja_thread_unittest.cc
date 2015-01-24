// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "thread/ninja_thread_impl.h"

class NinjaThreadTest : public testing::Test {
 protected:
  void SetUp() override {
    main_thread_.reset(new NinjaThreadImpl(NinjaThread::MAIN));
    io_thread_.reset(new NinjaThreadImpl(NinjaThread::RPC));
    main_thread_->Start();
    io_thread_->Start();
  }

  void TearDown() override {
    main_thread_->Stop();
    io_thread_->Stop();
  }

  static void BasicFunction(base::MessageLoop* message_loop) {
    CHECK(NinjaThread::CurrentlyOn(NinjaThread::RPC));
    message_loop->PostTask(FROM_HERE, base::MessageLoop::QuitClosure());
  }

 private:
  scoped_ptr<NinjaThreadImpl> main_thread_;
  scoped_ptr<NinjaThreadImpl> io_thread_;

  base::MessageLoop message_loop_;
};

TEST_F(NinjaThreadTest, PostTask) {
  NinjaThread::PostTask(
      NinjaThread::RPC,
      FROM_HERE,
      base::Bind(&BasicFunction, base::MessageLoop::current()));
  base::MessageLoop::current()->Run();
}
