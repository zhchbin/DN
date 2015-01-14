// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "rpc/rpc_channel.h"

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "net/tcp_client_socket.h"
#include "proto/rpc_message.pb.h"

#include "net/net_util.h"
#include "net/net_errors.h"
#include "net/io_buffer.h"

namespace rpc {

RpcChannel::RpcChannel(const std::string& server_ip, uint16 port)
    : server_ip_(server_ip),
      port_(port) {
}

RpcChannel::~RpcChannel() {
}

void RpcChannel::Connect() {
  net::IPAddressNumber ip_number;
  LOG(INFO) << server_ip_;
  net::ParseIPLiteralToNumber(server_ip_, &ip_number);
  net::AddressList address_list;
  address_list = net::AddressList::CreateFromIPAddress(ip_number, 20015);
  socket_.reset(new net::TCPClientSocket(address_list));
  net::CompletionCallback callback =
      base::Bind(&RpcChannel::OnConnectComplete, base::Unretained(this));
  int result = socket_->Connect(callback);
  if (result != net::ERR_IO_PENDING)
    callback.Run(result);
}

void RpcChannel::OnConnectComplete(int result) {
}

void RpcChannel::CallMethod(const pb::MethodDescriptor* method,
                            pb::RpcController* controller,
                            const pb::Message* request,
                            pb::Message* response,
                            pb::Closure* done) {
  scoped_ptr<RpcMessage> message(new RpcMessage());
  message->set_id(1);  // FIXME: Generate ID.
  message->set_type(RpcMessage::REQUEST);
  message->set_service(method->service()->full_name());
  message->set_method(method->name());
  message->set_request(request->SerializeAsString());

  // Connect to Rpc server.
  // Send the serialized data out.
  // Wait for the response.
  // Call done->Run().
}

}  // namespace rpc
