// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "slave/slave_main_runner.h"

#include "slave/command_executor.h"
#include "slave/slave_rpc.h"

SlaveMainRunner::SlaveMainRunner(const std::string& master, uint16 port)
    : master_(master),
      port_(port),
      command_executor_(new ninja::CommandExecutor()) {
}

SlaveMainRunner::~SlaveMainRunner() {
}

bool SlaveMainRunner::PostCreateThreads() {
  slave_rpc_.reset(new ninja::SlaveRPC(master_, port_, this));
  return true;
}

void SlaveMainRunner::Shutdown() {
}

void SlaveMainRunner::RunCommand(const std::string& command) {
  command_executor_->AppendCommand(command);
}
