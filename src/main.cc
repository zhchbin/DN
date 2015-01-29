// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "base/at_exit.h"
#include "base/command_line.h"
#include "common/main_runner.h"

int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  base::AtExitManager exit_manager;
  scoped_ptr<common::MainRunner> main_runner(common::MainRunner::Create());
  main_runner->CreateThreads();
  CHECK(main_runner->PostCreateThreads()) << "PostCreateThreads return false";
  main_runner->Run();
  main_runner->Shutdown();

  return 0;
}
