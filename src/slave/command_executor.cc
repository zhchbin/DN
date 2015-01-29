// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "slave/command_executor.h"

#include "base/bind.h"
#include "base/logging.h"
#include "thread/ninja_thread.h"

namespace ninja {

CommandExecutor::CommandExecutor() : weak_factory_(this) {}

CommandExecutor::~CommandExecutor() {}

void CommandExecutor::AppendCommand(const std::string& command) {
  incoming_command_queue_.push(command);
  StartCommand();
}

void CommandExecutor::CleanUp() {
  subprocs_.Clear();
}

int pending_commands = 0;

void CommandExecutor::StartCommand() {
  DCHECK(NinjaThread::CurrentlyOn(NinjaThread::MAIN));
  if (!incoming_command_queue_.empty() && CanRunMore()) {
    std::string command = incoming_command_queue_.front();
    incoming_command_queue_.pop();
    Subprocess* subproc = subprocs_.Add(command, true /*use console*/);
    CHECK(subproc != NULL);
    pending_commands++;
  } else {
    CommandRunner::Result result;
    while (pending_commands) {
      WaitForCommand(&result);
      pending_commands--;
    }
  }

  if (incoming_command_queue_.empty())
    return;

  // Start commands in the next message loop if possible.
  NinjaThread::PostTask(
      NinjaThread::MAIN,
      FROM_HERE,
      base::Bind(&CommandExecutor::StartCommand, weak_factory_.GetWeakPtr()));
  return;
}

bool CommandExecutor::WaitForCommand(CommandRunner::Result* result) {
  Subprocess* subproc;
  while ((subproc = subprocs_.NextFinished()) == NULL)
    subprocs_.DoWork();

  result->status = subproc->Finish();
  result->output = subproc->GetOutput();
  delete subproc;
  return true;
}

bool CommandExecutor::CanRunMore() {
  size_t subproc_number =
      subprocs_.running_.size() + subprocs_.finished_.size();

  return subproc_number < 4;
}

}  // namespace ninja
