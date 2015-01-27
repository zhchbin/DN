// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  NINJA_SLAVE_COMMAND_RUNNER_H_
#define  NINJA_SLAVE_COMMAND_RUNNER_H_

#include <string>
#include <queue>

#include "base/memory/ref_counted.h"
#include "third_party/ninja/src/build.h"
#include "third_party/ninja/src/subprocess.h"

namespace ninja {

class SlaveCommandRunner
    : public base::RefCountedThreadSafe<SlaveCommandRunner> {
 public:
  SlaveCommandRunner();

  void AppendCommand(const std::string& command);
  void CleanUp();

 private:
  friend class base::RefCountedThreadSafe<SlaveCommandRunner>;
  virtual ~SlaveCommandRunner();

  void StartCommand();
  bool WaitForCommand(CommandRunner::Result* result);
  bool CanRunMore();

  typedef std::queue<std::string> IncomingCommandQueue;
  IncomingCommandQueue incoming_command_queue_;

  // A set of async subprocess.
  SubprocessSet subprocs_;
  DISALLOW_COPY_AND_ASSIGN(SlaveCommandRunner);
};

}  // namespace ninja

#endif  // NINJA_SLAVE_COMMAND_RUNNER_H_
