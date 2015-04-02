// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "slave/command_executor.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "common/util.h"

namespace slave {

CommandExecutor::CommandExecutor()
  : parallelism_(common::GuessParallelism()),
    running_commands_(0) {
}

CommandExecutor::~CommandExecutor() {
}

void CommandExecutor::AddObserver(Observer* obs) {
  observer_list_.AddObserver(obs);
}

void CommandExecutor::RemoveObserver(Observer* obs) {
  observer_list_.RemoveObserver(obs);
}

void CommandExecutor::RunCommand(const std::string& command) {
  if (running_commands_ <= parallelism_) {
    // |proc| will be delete when subprocess exists.
    common::AsyncSubprocess* proc = new common::AsyncSubprocess;
    CHECK(proc->Start(command,
                      base::Bind(&CommandExecutor::SubprocessExitCallback,
                                 base::Unretained(this))));
    FOR_EACH_OBSERVER(Observer, observer_list_, OnCommandStarted(command));
    subprocss_to_command_.insert(std::make_pair(proc, command));
    ++running_commands_;
  } else {
    pending_command_queue_.push(command);
  }
}

void CommandExecutor::SubprocessExitCallback(common::AsyncSubprocess* subproc) {
  CommandRunner::Result result;
  SubprocessToCommand::iterator it = subprocss_to_command_.find(subproc);
  DCHECK(it != subprocss_to_command_.end());
  result.status = subproc->Finish();
  result.output = subproc->GetOutput();
  FOR_EACH_OBSERVER(
      Observer, observer_list_, OnCommandFinished(it->second, &result));

  subprocss_to_command_.erase(it);
  delete subproc;
  --running_commands_;

  if (!pending_command_queue_.empty()) {
    // Don't call RunCommand directlly since we are in the callback of libevent.
    base::MessageLoop::current()->PostTask(FROM_HERE,
        base::Bind(&CommandExecutor::RunCommand,
                   base::Unretained(this),
                   pending_command_queue_.front()));
    pending_command_queue_.pop();
  }
}

}  // namespace slave
