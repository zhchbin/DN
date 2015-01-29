// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  SLAVE_SLAVE_MAIN_RUNNER_H_
#define  SLAVE_SLAVE_MAIN_RUNNER_H_

#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "common/main_runner.h"

namespace ninja {
class CommandExecutor;
class SlaveRPC;
}

class SlaveMainRunner
    : public common::MainRunner,
      public base::RefCountedThreadSafe<SlaveMainRunner> {
 public:
  SlaveMainRunner(const std::string& master, uint16 port);

  bool PostCreateThreads() override;
  void Shutdown() override;

  void RunCommand(const std::string& command);

 private:
  friend class base::RefCountedThreadSafe<SlaveMainRunner>;
  ~SlaveMainRunner() override;
  std::string master_;
  uint16 port_;

  scoped_ptr<ninja::SlaveRPC> slave_rpc_;
  scoped_ptr<ninja::CommandExecutor> command_executor_;

  DISALLOW_COPY_AND_ASSIGN(SlaveMainRunner);
};

#endif  // SLAVE_SLAVE_MAIN_RUNNER_H_
