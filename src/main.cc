// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include <string>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "common/main_runner.h"

int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  base::AtExitManager exit_manager;

  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();
  static const char kWorkingDir[] = "working_dir";
  if (command_line->HasSwitch(kWorkingDir)) {
    std::string working_dir = command_line->GetSwitchValueASCII(kWorkingDir);
    if (!working_dir.empty()) {
      LOG(INFO) << "dn: Entering directory " << working_dir;
      int ret = chdir(working_dir.c_str());
      CHECK(ret == 0) << "chdir to " << working_dir << " -" << strerror(errno);
    }
  }

  scoped_ptr<common::MainRunner> main_runner(common::MainRunner::Create());
  std::string error;
  static const char kDefaultManifest[] = "build.ninja";
  CHECK(main_runner->InitFromManifest(kDefaultManifest, &error)) << error;
  main_runner->CreateThreads();
  CHECK(main_runner->PostCreateThreads()) << "PostCreateThreads return false";
  main_runner->Run();
  main_runner->Shutdown();

  return 0;
}
