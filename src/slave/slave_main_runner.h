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
#include "slave/command_executor.h"

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

class SlaveMainRunner : public CommandExecutor::Observer,
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

 private:
  friend class base::RefCountedThreadSafe<SlaveMainRunner>;
  ~SlaveMainRunner() override;

  // Create directories necessary for outputs and create response file,
  // if needed. Note: this will block.
  bool CreateDirsAndResponseFile(const RunCommandRequest* request);

  std::string master_;
  uint16 port_;

  scoped_ptr<SlaveRPC> slave_rpc_;
  scoped_ptr<CommandExecutor> command_executor_;
  scoped_ptr<SlaveFileThread> slave_file_thread_;

  typedef std::set<uint32> NinjaCommmandHashSet;
  NinjaCommmandHashSet ninja_command_hash_set_;

  typedef std::pair<
      RunCommandResponse*, google::protobuf::Closure*> ResponsePair;
  typedef std::map<uint32, ResponsePair> HashToResponsePair;
  HashToResponsePair hash_to_response_pair_;

  DISALLOW_COPY_AND_ASSIGN(SlaveMainRunner);
};

}  // namespace slave

#endif  // SLAVE_SLAVE_MAIN_RUNNER_H_
