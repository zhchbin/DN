// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  SLAVE_SLAVE_MAIN_RUNNER_H_
#define  SLAVE_SLAVE_MAIN_RUNNER_H_

#include <map>
#include <set>
#include <string>
#include <utility>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "common/main_runner.h"
#include "slave/command_executor.h"

namespace ninja {
class SlaveRPC;
}  // namespace ninja

namespace slave {
class RunCommandRequest;
class RunCommandResponse;
}  // namespace slave

namespace google {
namespace protobuf {
class Closure;
}  // namespace protobuf
}  // namespace google

class SlaveMainRunner
    :  public base::RefCountedThreadSafe<SlaveMainRunner>,
       public ninja::CommandExecutor::Observer,
       public common::MainRunner {
 public:
  SlaveMainRunner(const std::string& master, uint16 port);

  // ninja::CommandExecutor::Observer implementations.
  void OnCommandStarted(const std::string& command) override;
  void OnCommandFinished(const std::string& command,
                         const CommandRunner::Result* result) override;

  // common::MainRunner implementations.
  bool PostCreateThreads() override;
  void Shutdown() override;

  void RunCommand(const ::slave::RunCommandRequest* request,
                  ::slave::RunCommandResponse* response,
                  ::google::protobuf::Closure* done);

 private:
  friend class base::RefCountedThreadSafe<SlaveMainRunner>;
  ~SlaveMainRunner() override;
  std::string master_;
  uint16 port_;

  scoped_ptr<ninja::SlaveRPC> slave_rpc_;
  scoped_ptr<ninja::CommandExecutor> command_executor_;

  typedef std::set<uint32> NinjaCommmandHashSet;
  NinjaCommmandHashSet ninja_command_hash_set_;

  typedef std::pair<
      ::slave::RunCommandResponse*, ::google::protobuf::Closure*> ResponsePair;
  typedef std::map<uint32, ResponsePair> HashToResponsePair;
  HashToResponsePair hash_to_response_pair_;

  DISALLOW_COPY_AND_ASSIGN(SlaveMainRunner);
};

#endif  // SLAVE_SLAVE_MAIN_RUNNER_H_
