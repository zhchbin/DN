// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  MASTER_MASTER_MAIN_RUNNER_H_
#define  MASTER_MASTER_MAIN_RUNNER_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "common/main_runner.h"
#include "thread/ninja_thread_delegate.h"

namespace master {

class MasterRPC;

class MasterMainRunner
    : public base::RefCountedThreadSafe<MasterMainRunner>,
      public common::MainRunner {
 public:
  MasterMainRunner(const std::string& bind_ip, uint16 port);

  // common::MainRunner implementations.
  bool PostCreateThreads() override;
  void Shutdown() override;

 private:
  friend class base::RefCountedThreadSafe<MasterMainRunner>;
  ~MasterMainRunner() override;

  std::string bind_ip_;
  uint16 port_;
  scoped_ptr<MasterRPC> master_rpc_;

  DISALLOW_COPY_AND_ASSIGN(MasterMainRunner);
};

}  // namespace master

#endif  // MASTER_MASTER_MAIN_RUNNER_H_
