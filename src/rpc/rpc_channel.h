// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  RPC_RPC_CHANNEL_H_
#define  RPC_RPC_CHANNEL_H_

#include <map>
#include <string>
#include <utility>

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "google/protobuf/service.h"
#include "rpc/rpc_connection.h"

namespace google {
namespace protobuf {

class MethodDescriptor;
class RpcController;
class Messae;
class Closure;

}  // namespace protobuf
}  // namespace google

namespace net {
class TCPClientSocket;
}  // namespace net

namespace pb = google::protobuf;

namespace rpc {

class RpcChannel : public pb::RpcChannel {
 public:
  RpcChannel(const std::string& server_ip, uint16 port);
  virtual ~RpcChannel();

  // Call the given method of the remote service.  The signature of this
  // procedure looks the same as Service::CallMethod(), but the requirements
  // are less strict in one important way:  the request and response objects
  // need not be of any specific class as long as their descriptors are
  // method->input_type() and method->output_type().
  void CallMethod(const pb::MethodDescriptor* method,
                  pb::RpcController* controller,
                  const pb::Message* request,
                  pb::Message* response,
                  pb::Closure* done) override;

  void Connect();

  void SendEcho();

 private:
  // The first one is the response message pointer, the second is the callback
  // closure.
  typedef std::pair<pb::Message*, pb::Closure*> Response;
  typedef std::map<int, Response> RequsetIdToResponseMap;

  void OnConnectComplete(int result);
  void OnWriteComplete(int result);

  void OnReadCompleted(int result);

  std::string server_ip_;
  uint16 port_;
  scoped_ptr<net::TCPClientSocket> socket_;
  scoped_refptr<RpcConnection::ReadIOBuffer> read_buffer_;
  bool is_connected_;
  uint32 last_id_;
  RequsetIdToResponseMap request_id_to_response_map_;

  base::WeakPtrFactory<RpcChannel> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(RpcChannel);
};

}  // namespace rpc

#endif  // RPC_RPC_CHANNEL_H_
