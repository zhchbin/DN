// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "ninja/slave_command_runner.h"

#include "base/bind.h"
#include "base/logging.h"
#include "thread/ninja_thread.h"

namespace ninja {

SlaveCommandRunner::SlaveCommandRunner() {}

SlaveCommandRunner::~SlaveCommandRunner() {}

void SlaveCommandRunner::AppendCommand(const std::string& command) {
  incoming_command_queue_.push(command);
  StartCommand();
}

void SlaveCommandRunner::CleanUp() {
  subprocs_.Clear();
}

int pending_commands = 0;

void SlaveCommandRunner::StartCommand() {
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
      base::Bind(&SlaveCommandRunner::StartCommand, this));
  return;
}

bool SlaveCommandRunner::WaitForCommand(CommandRunner::Result* result) {
  Subprocess* subproc;
  while ((subproc = subprocs_.NextFinished()) == NULL)
    subprocs_.DoWork();

  result->status = subproc->Finish();
  result->output = subproc->GetOutput();
  delete subproc;
  return true;
}

bool SlaveCommandRunner::CanRunMore() {
  size_t subproc_number =
      subprocs_.running_.size() + subprocs_.finished_.size();

  return subproc_number < 4;
}

}  // namespace ninja
