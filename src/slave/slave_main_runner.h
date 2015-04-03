// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  SLAVE_SLAVE_MAIN_RUNNER_H_
#define  SLAVE_SLAVE_MAIN_RUNNER_H_

#include <map>
#include <set>
#include <string>
#include <utility>

#include "common/main_runner.h"
#include "common/command_executor.h"
#include "third_party/ninja/src/build.h"

namespace slave {
class RunCommandRequest;
class RunCommandResponse;
}  // namespace slave

namespace google {
namespace protobuf {
class Closure;
}  // namespace protobuf
}  // namespace google

namespace slave {

class SlaveFileThread;
class SlaveRPC;

class SlaveMainRunner : public common::CommandExecutor::Observer,
                        public common::MainRunner {
 public:
  SlaveMainRunner(const std::string& master, uint16 port);

  // slave::CommandExecutor::Observer implementations.
  void OnCommandStarted(const std::string& command) override;
  void OnCommandFinished(const std::string& command,
                         const CommandRunner::Result* result) override;

  // common::MainRunner implementations.
  bool PostCreateThreads() override;

  void RunCommand(const RunCommandRequest* request,
                  RunCommandResponse* response,
                  google::protobuf::Closure* done);
  void Wait();

 private:
  struct RunCommandContext {
    const RunCommandRequest* request;
    RunCommandResponse* response;
    google::protobuf::Closure* done;
  };

  friend class base::RefCountedThreadSafe<SlaveMainRunner>;
  ~SlaveMainRunner() override;

  // Create directories necessary for outputs and create response file,
  // if needed. Note: this will block.
  bool CreateDirsAndResponseFile(const RunCommandRequest* request);

  void MD5OutputsOnBlockingPool(const RunCommandContext& context);

  bool StartEdge(Edge* edge);

  std::string master_;
  uint16 port_;

  scoped_ptr<SlaveRPC> slave_rpc_;
  scoped_ptr<common::CommandExecutor> command_executor_;
  scoped_ptr<SlaveFileThread> slave_file_thread_;

  typedef std::map<uint32, Edge*> HashEdgeMap;
  HashEdgeMap hash_edge_map_;

  std::set<Edge*> started_edge_set_;

  // RunCommandContextMap is used to hold the context of running a command from
  // master. Key is the hash of |request->command()|.
  typedef std::map<uint32, RunCommandContext> RunCommandContextMap;
  RunCommandContextMap run_command_context_map_;

  DISALLOW_COPY_AND_ASSIGN(SlaveMainRunner);
};

}  // namespace slave

#endif  // SLAVE_SLAVE_MAIN_RUNNER_H_
