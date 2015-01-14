// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  RPC_RPC_CLIENT_MAIN_H_
#define  RPC_RPC_CLIENT_MAIN_H_

#include <string>

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "ninja_thread_delegate.h"  // NOLINT

namespace rpc {

class RpcChannel;

class RpcClientMain : public NinjaThreadDelegate {
 public:
  RpcClientMain(const std::string& server_ip, uint16 port);
  virtual ~RpcClientMain();

  // NinjaThreadDelegate implementations.
  void Init() override;
  void InitAsync() override;
  void CleanUp() override;

 private:
  std::string server_ip_;
  uint16 port_;
  scoped_ptr<rpc::RpcChannel> rpc_channel_;

  DISALLOW_COPY_AND_ASSIGN(RpcClientMain);
};

}  // namespace rpc

#endif  // RPC_RPC_CLIENT_MAIN_H_
