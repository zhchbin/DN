// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "rpc/rpc_channel.h"

#include "base/big_endian.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "net/io_buffer.h"
#include "net/net_errors.h"
#include "net/net_util.h"
#include "net/tcp_client_socket.h"
#include "proto/rpc_message.pb.h"

namespace {
static const int32 kOneKilobyte = 1024;
static const int32 kOneMegabyte = 1024 * kOneKilobyte;
static const uint8 kSizeOfUint32 = 4;
}  // namespace

namespace rpc {

RpcChannel::RpcChannel(const std::string& server_ip, uint16 port)
    : server_ip_(server_ip),
      port_(port),
      read_buffer_(new RpcConnection::ReadIOBuffer()),
      is_connected_(false),
      last_id_(0),
      weak_ptr_factory_(this) {
  read_buffer_->SetCapacity(kOneKilobyte);
}

RpcChannel::~RpcChannel() {
}

void RpcChannel::Connect() {
  net::IPAddressNumber ip_number;
  net::ParseIPLiteralToNumber(server_ip_, &ip_number);
  net::AddressList address_list;
  address_list = net::AddressList::CreateFromIPAddress(ip_number, 20015);
  socket_.reset(new net::TCPClientSocket(address_list));
  int result = socket_->Connect(
      base::Bind(&RpcChannel::OnConnectComplete, base::Unretained(this)));
  if (result != net::ERR_IO_PENDING)
    OnConnectComplete(result);
}

void RpcChannel::CallMethod(const pb::MethodDescriptor* method,
                            pb::RpcController* controller,
                            const pb::Message* request,
                            pb::Message* response,
                            pb::Closure* done) {
  DCHECK(is_connected_);
  scoped_ptr<RpcMessage> message(new RpcMessage());
  message->set_id(last_id_++);
  message->set_type(RpcMessage::REQUEST);
  message->set_service(method->service()->full_name());
  message->set_method(method->name());
  message->set_request(request->SerializeAsString());
  request_id_to_response_map_[message->id()] = std::make_pair(response, done);

  // Message format:
  //                   _
  //   |-------------|  |
  //   |    length   |  |
  //   |-------------|  |
  //   |             |  |
  //   |   protobuf  |   >  |length| bytes
  //   |   message   |  |
  //   |    data     |  |
  //   |             |  |
  //   |-------------| _|
  //
  // Message length: type is uint32, 4 Bytes.
  //
  scoped_refptr<net::IOBuffer> write_buffer(
      new net::IOBuffer(kSizeOfUint32 + message->ByteSize()));
  base::WriteBigEndian(write_buffer->data(),
                       static_cast<uint32>(message->ByteSize()));
  message->SerializeToArray(write_buffer->data() + kSizeOfUint32,
                            message->ByteSize());
  socket_->Write(
      write_buffer.get(),
      message->ByteSize() + kSizeOfUint32,
      base::Bind(&RpcChannel::OnWriteComplete, base::Unretained(this)));

  // Make sure we can read the whole message.
  if (read_buffer_->RemainingCapacity() <= kOneMegabyte)
    read_buffer_->IncreaseCapacity();
  int result = socket_->Read(read_buffer_.get(),
                             read_buffer_->RemainingCapacity(),
                             base::Bind(&RpcChannel::OnReadCompleted,
                                        base::Unretained(this)));
  if (result != net::ERR_IO_PENDING)
    OnReadCompleted(result);
}

void RpcChannel::OnConnectComplete(int result) {
  is_connected_ = result == net::OK;

  if (is_connected_)
    socket_->SetSendBufferSize(kOneMegabyte);
}

void RpcChannel::OnWriteComplete(int result) {
  LOG(INFO) << "OnWriteComplete" << result;
}

void RpcChannel::OnReadCompleted(int result) {
  if (result < 0)
    return;

  read_buffer_->DidRead(result);
  while (read_buffer_->GetSize() > 0) {
    uint32 message_length;
    base::ReadBigEndian(read_buffer_->StartOfBuffer(), &message_length);
    if (static_cast<uint32>(read_buffer_->GetSize()) <
        (kSizeOfUint32 + message_length)) {
      break;
    }

    rpc::RpcMessage message;
    if (message.ParseFromArray(read_buffer_->StartOfBuffer() + kSizeOfUint32,
                               message_length)) {
      read_buffer_->DidConsume(kSizeOfUint32 + message_length);
      RequsetIdToResponseMap::const_iterator it =
          request_id_to_response_map_.find(message.id());
      if (it == request_id_to_response_map_.end()) {
        LOG(ERROR) << "Unknown request id received.";
        continue;
      }

      Response response = it->second;
      DCHECK_EQ(message.type(), RpcMessage::RESPONSE);

      response.first->ParseFromString(message.response());
      if (response.second)
        response.second->Run();
      request_id_to_response_map_.erase(it);
    }
  }
}

}  // namespace rpc
