// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  RPC_RPC_SERVER_H_
#define  RPC_RPC_SERVER_H_

#include <map>
#include <string>

#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/service.h"

namespace net {

class ServerSocket;
class StreamSocket;

}  // namespace net

namespace pb = google::protobuf;

namespace rpc {

class RpcConnection;

class RpcServer {
 public:
  explicit RpcServer(scoped_ptr<net::ServerSocket> server_socket);
  ~RpcServer();

  void Close(int connection_id);

  typedef std::map<std::string, pb::Service*> ServiceMap;
  ServiceMap service_map_;

  void RegisterService(pb::Service *service) {
    // Don't Register same service twice.
    DCHECK(service_map_.find(service->GetDescriptor()->full_name()) ==
           service_map_.end());
    service_map_[service->GetDescriptor()->full_name()] = service;
  }

 private:
  typedef std::map<int, RpcConnection*> IdToConnectionMap;

  struct RequestParameters {
    RequestParameters(int connection_id,
                      pb::Message* request,
                      pb::Message* response,
                      uint64 request_id,
                      const std::string& service,
                      const std::string& method)
        : connection_id(connection_id),
          request(request),
          response(response),
          request_id(request_id),
          service(service),
          method(method) {
    }

    int connection_id;
    scoped_ptr<pb::Message> request;
    scoped_ptr<pb::Message> response;
    uint64 request_id;
    std::string service;
    std::string method;
  };

  void DoAcceptLoop();
  void OnAcceptCompleted(int rv);
  int HandleAcceptResult(int rv);

  void DoReadLoop(RpcConnection* connection);
  void OnReadCompleted(int connection_id, int rv);
  int HandleReadResult(RpcConnection* connection, int rv);

  void DoWriteLoop(RpcConnection* connection);
  void OnWriteCompleted(int connection_id, int rv);
  int HandleWriteResult(RpcConnection* connection, int rv);

  void OnServiceDone(RequestParameters* parameters);

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
