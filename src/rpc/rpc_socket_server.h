// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  RPC_RPC_SOCKET_SERVER_H_
#define  RPC_RPC_SOCKET_SERVER_H_

#include <map>
#include <string>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "rpc/rpc_connection.h"

namespace net {

class ServerSocket;
class StreamSocket;

}  // namespace net

namespace rpc {

class RpcSocketServer : public RpcConnection::Delegate {
 public:
  class Observer {
   public:
    virtual ~Observer() {}
    virtual void OnConnect(RpcConnection* connection) = 0;
    virtual void OnClose(RpcConnection* connection) = 0;
  };

  explicit RpcSocketServer(const std::string& bind_ip, uint16 port);
  ~RpcSocketServer();
  void AddObserver(Observer *obs);
  void RemoveObserver(Observer *obs);

  RpcConnection* FindConnection(int connection_id);

  // RpcConnection::Delegate implementations.
  void OnClose(RpcConnection* connection) override;

 private:
  typedef std::map<int, RpcConnection*> IdToConnectionMap;

  void Init();
  void DoAcceptLoop();
  void OnAcceptCompleted(int rv);
  int HandleAcceptResult(int rv);

  scoped_ptr<net::ServerSocket> server_socket_;
  scoped_ptr<net::StreamSocket> accepted_socket_;
  int last_id_;
  IdToConnectionMap id_to_connection_;
  std::string bind_ip_;
  uint16 port_;

  ObserverList<Observer> observer_list_;

  base::WeakPtrFactory<RpcSocketServer> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(RpcSocketServer);
};

}  // namespace rpc

#endif  // RPC_RPC_SOCKET_SERVER_H_
