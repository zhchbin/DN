// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  MASTER_MASTER_MAIN_RUNNER_H_
#define  MASTER_MASTER_MAIN_RUNNER_H_

#include <map>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "common/main_runner.h"
#include "third_party/ninja/src/build.h"
#include "third_party/ninja/src/subprocess.h"
#include "thread/ninja_thread_delegate.h"

namespace master {

class MasterRPC;

class MasterMainRunner : public common::MainRunner {
 public:
  MasterMainRunner(const std::string& bind_ip, uint16 port);

  // common::MainRunner implementations.
  bool PostCreateThreads() override;

  void StartBuild();

  bool LocalCanRunMore();
  bool RemoteCanRunMore();
  bool StartCommand(Edge* edge, bool run_in_local);
  bool WaitForCommand(CommandRunner::Result* result);
  std::vector<Edge*> GetActiveEdges();
  void Abort();
  bool HasPendingLocalCommands();

  void OnRemoteCommandDone(uint32 edge_id,
                           ExitStatus status,
                           const std::string& output);

  void OnCurlTargetDone(CommandRunner::Result result);

 private:
  friend class base::RefCountedThreadSafe<MasterMainRunner>;
  ~MasterMainRunner() override;

  bool StartCommandLocally(Edge* edge);
  bool StartCommandRemotely(Edge* edge);

  std::string bind_ip_;
  uint16 port_;
  scoped_ptr<MasterRPC> master_rpc_;

  BuildConfig config_;
  SubprocessSet subprocs_;
  typedef std::map<Subprocess*, Edge*> SubprocessToEdgeMap;
  SubprocessToEdgeMap subproc_to_edge_;

  typedef std::map<uint32, Edge*> OutstandingEdgeMap;
  OutstandingEdgeMap outstanding_edges_;

  DISALLOW_COPY_AND_ASSIGN(MasterMainRunner);
};

}  // namespace master

#endif  // MASTER_MASTER_MAIN_RUNNER_H_
