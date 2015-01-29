// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  COMMON_MAIN_RUNNER_H_
#define  COMMON_MAIN_RUNNER_H_

#include "base/message_loop/message_loop.h"
#include "thread/ninja_thread_impl.h"

namespace common {

// This class is responsible for dn master/slave initialization, running and
// shutdown.
class MainRunner {
 public:
  virtual ~MainRunner();

  static MainRunner* Create();

  void CreateThreads();
  virtual bool PostCreateThreads() = 0;
  void Run();
  virtual void Shutdown() = 0;

 private:
  scoped_ptr<NinjaThreadImpl> main_thread_;
  scoped_ptr<NinjaThreadImpl> rpc_thread_;

  base::MessageLoop message_loop_;
};

}  // namespace common

#endif  // COMMON_MAIN_RUNNER_H_
