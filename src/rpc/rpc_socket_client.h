// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  RPC_RPC_SOCKET_CLIENT_H_
#define  RPC_RPC_SOCKET_CLIENT_H_

#include <string>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "rpc/rpc_connection.h"

namespace net {
class StreamSocket;
}

namespace rpc {

class RpcMessage;

class RpcSocketClient {
 public:
  explicit RpcSocketClient(const std::string& server_ip, uint16 port);
  ~RpcSocketClient();

  void Connect();

  RpcConnection* connection() { return rpc_connection_.get(); }

 private:
  void OnConnectComplete(scoped_ptr<net::StreamSocket> socket, int result);

  std::string server_ip_;
  uint16 port_;
  scoped_ptr<RpcConnection> rpc_connection_;
  RpcConnection::MessageDelegate* message_delegate_;

  base::WeakPtrFactory<RpcSocketClient> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(RpcSocketClient);
};

}  // namespace rpc

#endif  // RPC_RPC_SOCKET_CLIENT_H_
