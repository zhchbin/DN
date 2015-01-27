// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "ninja/slave_command_runner.h"

#include "base/logging.h"
#include "thread/ninja_thread.h"

namespace ninja {

SlaveCommandRunner::SlaveCommandRunner() {}

SlaveCommandRunner::~SlaveCommandRunner() {}

void SlaveCommandRunner::StartCommand(const std::string& command) {
  DCHECK(NinjaThread::CurrentlyOn(NinjaThread::MAIN));
  Subprocess* subproc = subprocs_.Add(command, true /*use console*/);
  CHECK(subproc != NULL);
  return;
}

void SlaveCommandRunner::WaitForCommand() {
  Subprocess* subproc;
  while ((subproc = subprocs_.NextFinished()) == NULL) {
    bool interrupted = subprocs_.DoWork();
    if (interrupted) {
      return;
    }
  }

  LOG(INFO) << subproc->Finish();
  LOG(INFO) << subproc->GetOutput();

  delete subproc;
  return;
}

}  // namespace ninja
