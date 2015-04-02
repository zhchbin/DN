// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  SLAVE_COMMAND_EXECUTOR_H_
#define  SLAVE_COMMAND_EXECUTOR_H_

#include <map>
#include <queue>
#include <string>

#include "base/basictypes.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "common/async_subprocess.h"
#include "third_party/ninja/src/build.h"
#include "third_party/ninja/src/subprocess.h"

namespace slave {

class CommandExecutor {
 public:
  // Observer interface for notifying command started/finished event.
  class Observer {
   public:
    virtual ~Observer() {}
    virtual void OnCommandStarted(const std::string& command) = 0;
    virtual void OnCommandFinished(const std::string& command,
                                   const CommandRunner::Result* result) = 0;
  };

  CommandExecutor();
  ~CommandExecutor();

  void AddObserver(Observer* obs);
  void RemoveObserver(Observer* obs);

  void RunCommand(const std::string& command);
  void SubprocessExitCallback(common::AsyncSubprocess* subproc);

 private:
  int parallelism_;
  int running_commands_;

  typedef std::map<common::AsyncSubprocess*, std::string> SubprocessToCommand;
  SubprocessToCommand subprocss_to_command_;

  ObserverList<Observer> observer_list_;

  typedef std::queue<std::string> PendingCommandQueue;
  PendingCommandQueue pending_command_queue_;

  DISALLOW_COPY_AND_ASSIGN(CommandExecutor);
};

}  // namespace slave

#endif  // SLAVE_COMMAND_EXECUTOR_H_
