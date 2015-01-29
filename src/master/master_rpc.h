// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  MASTER_MASTER_RPC_H_
#define  MASTER_MASTER_RPC_H_

#include <string>

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "thread/ninja_thread_delegate.h"

namespace rpc {
class RpcSocketServer;
}

namespace ninja {

class MasterRPC : public NinjaThreadDelegate {
 public:
  MasterRPC(const std::string& bind_ip, uint16 port);
  virtual ~MasterRPC();

  // NinjaThreadDelegate implementations.
  void Init() override;
  void InitAsync() override;
  void CleanUp() override;

 private:
  std::string bind_ip_;
  uint16 port_;
  scoped_ptr<rpc::RpcSocketServer> rpc_socket_server_;

  DISALLOW_COPY_AND_ASSIGN(MasterRPC);
};

}  // namespace ninja

#endif  // MASTER_MASTER_RPC_H_
