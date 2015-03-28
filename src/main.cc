// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include <string>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/json/json_file_value_serializer.h"
#include "base/strings/string_number_conversions.h"
#include "common/main_runner.h"
#include "common/options.h"

#if defined(OS_WIN)
#include <direct.h>
#define chdir _chdir
#endif

namespace {
static const char kWorkingDir[] = "working_dir";
}   // namespace

void ReadSwitchesFromConfig() {
  base::FilePath config_path;
  PathService::Get(base::DIR_CURRENT, &config_path);
  config_path = config_path.AppendASCII("dn_config.json");
  if (!base::PathExists(config_path))
    return;

  std::string error;
  JSONFileValueSerializer serializer(config_path);
  scoped_ptr<base::Value> root(serializer.Deserialize(NULL, &error));
  if (!root->IsType(base::Value::TYPE_DICTIONARY))
    return;

  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  scoped_ptr<base::DictionaryValue> values(
      static_cast<base::DictionaryValue*>(root.release()));

  std::string working_dir;
  if (values->GetString(kWorkingDir, &working_dir))
    command_line->AppendSwitchASCII(kWorkingDir, working_dir);

  std::string bind_ip;
  if (values->GetString(switches::kBindIP, &bind_ip))
    command_line->AppendSwitchASCII(switches::kBindIP, bind_ip);

  std::string port;
  if (values->GetString(switches::kPort, &port))
    command_line->AppendSwitchASCII(switches::kPort, port);

  std::string master;
  if (values->GetString(switches::kMaster, &master))
    command_line->AppendSwitchASCII(switches::kMaster, master);

  std::string targets;
  if (values->GetString(switches::kTargets, &targets))
    command_line->AppendSwitchASCII(switches::kTargets, targets);

  int slave_amount;
  if (values->GetInteger(switches::kSlaveAmount, &slave_amount)) {
    command_line->AppendSwitchASCII(switches::kSlaveAmount,
                                    base::IntToString(slave_amount));
  }
}

int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  base::AtExitManager exit_manager;
  ReadSwitchesFromConfig();

  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(kWorkingDir)) {
    std::string working_dir = command_line->GetSwitchValueASCII(kWorkingDir);
    if (!working_dir.empty()) {
      LOG(INFO) << "dn: Entering directory " << working_dir;
      int ret = chdir(working_dir.c_str());
      CHECK(ret == 0) << "chdir to " << working_dir << " -" << strerror(errno);
    }
  }

  scoped_refptr<common::MainRunner> main_runner(common::MainRunner::Create());
  std::string error;
  static const char kDefaultManifest[] = "build.ninja";
  CHECK(main_runner->InitFromManifest(kDefaultManifest, &error)) << error;
  main_runner->CreateThreads();
  CHECK(main_runner->PostCreateThreads()) << "PostCreateThreads return false";
  main_runner->Run();
  main_runner->Shutdown();

  return 0;
}
