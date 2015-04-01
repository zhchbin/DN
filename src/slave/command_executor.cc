// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "slave/command_executor.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "common/util.h"

namespace slave {

CommandExecutor::CommandExecutor() {}

CommandExecutor::~CommandExecutor() {}

void CommandExecutor::AddObserver(Observer* obs) {
  observer_list_.AddObserver(obs);
}

void CommandExecutor::RemoveObserver(Observer* obs) {
  observer_list_.RemoveObserver(obs);
}

void CommandExecutor::CleanUp() {
  subprocs_.Clear();
}

void CommandExecutor::RunCommand(const std::string& command) {
  Subprocess* subproc = subprocs_.Add(command, false /*use console*/);
  CHECK(subproc != NULL);
  FOR_EACH_OBSERVER(Observer, observer_list_, OnCommandStarted(command));
  subprocss_to_command_.insert(std::make_pair(subproc, command));
}

void CommandExecutor::Wait() {
  if (subprocss_to_command_.empty())
    return;

  while (subprocs_.finished_.empty())
    subprocs_.DoWork();

  scoped_ptr<CommandRunner::Result> result(new CommandRunner::Result);
  Subprocess* subproc = NULL;
  while ((subproc = subprocs_.NextFinished()) != NULL) {
    result->status = subproc->Finish();
    result->output = subproc->GetOutput();
    SubprocessToCommand::iterator it = subprocss_to_command_.find(subproc);
    DCHECK(it != subprocss_to_command_.end());
    FOR_EACH_OBSERVER(
        Observer, observer_list_, OnCommandFinished(it->second, result.get()));
    subprocss_to_command_.erase(it);
    delete subproc;
    subproc = NULL;
  }
}

}  // namespace slave
