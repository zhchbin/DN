// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  MASTER_MASTER_MAIN_RUNNER_H_
#define  MASTER_MASTER_MAIN_RUNNER_H_

#include <string>

#include "base/macros.h"
#include "common/main_runner.h"

namespace ninja {
class MasterRPC;
}

class MasterMainRunner : public common::MainRunner {
 public:
  MasterMainRunner(const std::string& bind_ip, uint16 port);
  ~MasterMainRunner() override;

  bool PostCreateThreads() override;
  void Shutdown() override;

 private:
  std::string bind_ip_;
  uint16 port_;

  scoped_ptr<ninja::MasterRPC> master_rpc_;

  DISALLOW_COPY_AND_ASSIGN(MasterMainRunner);
};

#endif  // MASTER_MASTER_MAIN_RUNNER_H_
