// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "master/master_main_runner.h"

#include "base/bind.h"
#include "master/master_rpc.h"

namespace master {

MasterMainRunner::MasterMainRunner(const std::string& bind_ip, uint16 port)
    : bind_ip_(bind_ip),
      port_(port) {
}

MasterMainRunner::~MasterMainRunner() {
}

bool MasterMainRunner::PostCreateThreads() {
  master_rpc_.reset(new MasterRPC(bind_ip_, port_, make_scoped_refptr(this)));
  ninja_builder()->RunBuild();

  return true;
}

void MasterMainRunner::Shutdown() {
}

}  // namespace master
