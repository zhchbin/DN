// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  RPC_RPC_SERVER_MAIN_H_
#define  RPC_RPC_SERVER_MAIN_H_

#include <string>

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "ninja_thread_delegate.h"  // NOLINT

namespace rpc {

class RpcServer;

class RpcServerMain : public NinjaThreadDelegate {
 public:
  RpcServerMain(const std::string& bind_ip, uint16 port);
  virtual ~RpcServerMain();

  // NinjaThreadDelegate implementations.
  void Init() override;
  void InitAsync() override;
  void CleanUp() override;

 private:
  void RegisterServices();

  std::string bind_ip_;
  uint16 port_;
  scoped_ptr<rpc::RpcServer> rpc_server_;

  DISALLOW_COPY_AND_ASSIGN(RpcServerMain);
};

}  // namespace rpc

#endif  // RPC_RPC_SERVER_MAIN_H_
