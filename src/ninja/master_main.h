// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  NINJA_MASTER_MAIN_H_
#define  NINJA_MASTER_MAIN_H_

#include <string>

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "rpc/service_manager.h"
#include "rpc/rpc_connection.h"
#include "thread/ninja_thread_delegate.h"

namespace google {
namespace protobuf {
class Message;
}
}

namespace rpc {
class RpcSocketServer;
}

namespace ninja {

class MasterMain : public NinjaThreadDelegate {
 public:
  MasterMain(const std::string& bind_ip, uint16 port);
  virtual ~MasterMain();

  // NinjaThreadDelegate implementations.
  void Init() override;
  void InitAsync() override;
  void CleanUp() override;

 private:
  std::string bind_ip_;
  uint16 port_;
  scoped_ptr<rpc::RpcSocketServer> rpc_socket_server_;

  DISALLOW_COPY_AND_ASSIGN(MasterMain);
};

}  // namespace ninja

#endif  // NINJA_MASTER_MAIN_H_
