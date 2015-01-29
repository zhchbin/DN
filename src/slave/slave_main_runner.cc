// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "slave/slave_main_runner.h"

#include "slave/slave_rpc.h"

SlaveMainRunner::SlaveMainRunner(const std::string& master, uint16 port)
    : master_(master),
      port_(port) {
}

SlaveMainRunner::~SlaveMainRunner() {
}

bool SlaveMainRunner::PostCreateThreads() {
  slave_rpc_.reset(new ninja::SlaveRPC(master_, port_));

  return true;
}

void SlaveMainRunner::Shutdown() {
}
