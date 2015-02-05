// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  COMMON_MAIN_RUNNER_H_
#define  COMMON_MAIN_RUNNER_H_

#include <string>

#include "base/message_loop/message_loop.h"
#include "ninja/ninja_main.h"
#include "thread/ninja_thread_impl.h"

namespace common {

// This class is responsible for dn master/slave initialization, running and
// shutdown.
class MainRunner : public base::RefCountedThreadSafe<MainRunner> {
 public:
  virtual ~MainRunner();
  static MainRunner* Create();

  // The following methods are called in the main routine in declaration order.
  bool InitFromManifest(const std::string& input_file, std::string* error);
  void CreateThreads();
  virtual bool PostCreateThreads() = 0;
  void Run();
  virtual void Shutdown();

 protected:
  ninja::NinjaMain* ninja_main() { return ninja_main_.get(); }

 private:
  friend class base::RefCountedThreadSafe<MainRunner>;

  scoped_ptr<NinjaThreadImpl> main_thread_;
  scoped_ptr<NinjaThreadImpl> rpc_thread_;
  scoped_ptr<NinjaThreadImpl> file_thread_;
  scoped_ptr<ninja::NinjaMain> ninja_main_;

  base::MessageLoop message_loop_;
};

}  // namespace common

#endif  // COMMON_MAIN_RUNNER_H_
