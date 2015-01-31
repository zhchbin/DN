// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  SLAVE_COMMAND_EXECUTOR_H_
#define  SLAVE_COMMAND_EXECUTOR_H_

#include <map>
#include <queue>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "third_party/ninja/src/build.h"
#include "third_party/ninja/src/subprocess.h"

namespace ninja {

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

  void AppendCommand(const std::string& command);
  void CleanUp();

 private:
  void StartCommand();
  bool WaitForCommand(CommandRunner::Result* result);
  bool CanRunMore();

  typedef std::queue<std::string> IncomingCommandQueue;
  IncomingCommandQueue incoming_command_queue_;

  typedef std::map<Subprocess*, std::string> SubprocessToCommand;
  SubprocessToCommand subprocss_to_command_;

  // A set of async subprocess.
  SubprocessSet subprocs_;

  ObserverList<Observer> observer_list_;

  base::WeakPtrFactory<CommandExecutor> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CommandExecutor);
};

}  // namespace ninja

#endif  // SLAVE_COMMAND_EXECUTOR_H_
