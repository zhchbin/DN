#ifndef  RPC_RPC_SERVER_H_
#define  RPC_RPC_SERVER_H_

#include <map>

#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"

namespace net{
class ServerSocket;
class StreamSocket;
}  // namespace net

namespace rpc {

class RpcConnection;

class RpcServer {
 public:
  RpcServer(scoped_ptr<net::ServerSocket> server_socket);
  ~RpcServer();

  void Close(int connection_id);

 private:
  typedef std::map<int, RpcConnection*> IdToConnectionMap;

  void DoAcceptLoop();
  void OnAcceptCompleted(int rv);
  int HandleAcceptResult(int rv);

  void DoReadLoop(RpcConnection* connection);
  void OnReadCompleted(int connection_id, int rv);
  int HandleReadResult(RpcConnection* connection, int rv);

  void DoWriteLoop(RpcConnection* connection);
  void OnWriteCompleted(int connection_id, int rv);
  int HandleWriteResult(RpcConnection* connection, int rv);

  RpcConnection* FindConnection(int connection_id);

  // Whether or not Close() has been called during delegate callback processing.
  bool HasConnectionClosed(RpcConnection* connection);

  const scoped_ptr<net::ServerSocket> server_socket_;
  scoped_ptr<net::StreamSocket> accepted_socket_;

  int last_id_;
  IdToConnectionMap id_to_connection_;

  base::WeakPtrFactory<RpcServer> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(RpcServer);
};

}  // namespace rpc

#endif  // RPC_RPC_SERVER_H_
