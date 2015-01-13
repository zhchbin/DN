// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  RPC_RPC_THREAD_DELEGATE_H_
#define  RPC_RPC_THREAD_DELEGATE_H_

#include "ninja_thread_delegate.h"

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"

namespace rpc {

class RpcServer;

class RpcThreadDelegate : public NinjaThreadDelegate {
 public:
  RpcThreadDelegate();
  virtual ~RpcThreadDelegate() override;

  // NinjaThreadDelegate implementations.
  virtual void Init() override;
  virtual void InitAsync() override;
  virtual void CleanUp() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(RpcThreadDelegate);
  scoped_ptr<rpc::RpcServer> rpc_server_;
};

}  // namespace rpc

#endif  // RPC_RPC_THREAD_DELEGATE_H_
