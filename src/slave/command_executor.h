// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  SLAVE_COMMAND_EXECUTOR_H_
#define  SLAVE_COMMAND_EXECUTOR_H_

#include <string>
#include <queue>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "third_party/ninja/src/build.h"
#include "third_party/ninja/src/subprocess.h"

namespace ninja {

class CommandExecutor {
 public:
  CommandExecutor();
  ~CommandExecutor();

  void AppendCommand(const std::string& command);
  void CleanUp();

 private:
  void StartCommand();
  bool WaitForCommand(CommandRunner::Result* result);
  bool CanRunMore();

  typedef std::queue<std::string> IncomingCommandQueue;
  IncomingCommandQueue incoming_command_queue_;

  // A set of async subprocess.
  SubprocessSet subprocs_;

  base::WeakPtrFactory<CommandExecutor> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CommandExecutor);
};

}  // namespace ninja

#endif  // SLAVE_COMMAND_EXECUTOR_H_
