// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  MASTER_MASTER_MAIN_RUNNER_H_
#define  MASTER_MASTER_MAIN_RUNNER_H_

#include <map>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "common/main_runner.h"
#include "third_party/ninja/src/build.h"
#include "third_party/ninja/src/subprocess.h"

namespace master {

class MasterRPC;
class WebUIThread;

struct SlaveInfo {
  int32 number_of_processors;
  int64 amount_of_physical_memory;
  int64 amount_of_virtual_memory;
  std::string operating_system_name;
  std::string operating_system_version;
  std::string operating_system_architecture;
  std::string ip;

  // The following fields will change dynamically.
  double load_average;
  int amount_of_running_commands;
  int64 amount_of_available_physical_memory;
};

class MasterMainRunner : public common::MainRunner {
 public:
  // The first one is the file path, the second one is its md5 digest.
  typedef std::pair<std::string, std::string> Target;
  typedef std::vector<Target> TargetVector;

  // Map the connection id to slave info.
  typedef std::map<int, SlaveInfo> SlaveInfoIdMap;

  MasterMainRunner(const std::string& bind_ip, uint16 port);

  // common::MainRunner implementations.
  bool PostCreateThreads() override;

  void StartBuild();

  bool StartEdgeRemotelly(Edge* edge, int connection_id);
  void BuildFinished();

  void OnRemoteCommandDone(int connection_id,
                           uint32 edge_id,
                           ExitStatus status,
                           const std::string& output,
                           const std::vector<std::string>& md5s);

  void FetchTargetsOnBlockingPool(const std::string& host,
                                  const TargetVector& targets,
                                  CommandRunner::Result result);
  void OnFetchTargetsDone(CommandRunner::Result result);

  void OnSlaveSystemInfoAvailable(int connection_id, const SlaveInfo& info);

  void OnSlaveStatusUpdate(int connection_id,
                           double load_average,
                           int amount_of_running_commands,
                           int64 amount_of_available_physical_memory);
  void OnSlaveClose(int connection_id);

  void SetWebUIInitialStatus(const std::string& json);
  void BuildEdgeStarted(Edge* edge);
  void BuildEdgeFinished(CommandRunner::Result* result);

  const SlaveInfoIdMap& GetSlaves() {
    return slave_info_id_map_;
  }

 private:
  SlaveInfoIdMap slave_info_id_map_;

  friend class base::RefCountedThreadSafe<MasterMainRunner>;
  ~MasterMainRunner() override;

  std::string bind_ip_;
  uint16 port_;
  scoped_ptr<MasterRPC> master_rpc_;

  BuildConfig config_;

  typedef std::map<uint32, Edge*> OutstandingEdgeMap;
  OutstandingEdgeMap outstanding_edges_;

  scoped_ptr<WebUIThread> webui_thread_;

  uint32 max_slave_amount_;
  bool is_building_;

  DISALLOW_COPY_AND_ASSIGN(MasterMainRunner);
};

}  // namespace master

#endif  // MASTER_MASTER_MAIN_RUNNER_H_
