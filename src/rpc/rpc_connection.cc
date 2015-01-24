// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "rpc/rpc_connection.h"

#include "base/logging.h"
#include "net/ip_endpoint.h"
#include "net/stream_socket.h"
#include "proto/rpc_message.pb.h"
#include "base/big_endian.h"
#include "net/net_errors.h"

#include "proto/echo.pb.h"

namespace rpc {

RpcConnection::ReadIOBuffer::ReadIOBuffer()
    : base_(new net::GrowableIOBuffer()),
      max_buffer_size_(kDefaultMaxBufferSize) {
  SetCapacity(kInitialBufSize);
}

RpcConnection::ReadIOBuffer::~ReadIOBuffer() {
  data_ = NULL;  // base_ owns data_.
}

int RpcConnection::ReadIOBuffer::GetCapacity() const {
  return base_->capacity();
}

void RpcConnection::ReadIOBuffer::SetCapacity(int capacity) {
  DCHECK_LE(GetSize(), capacity);
  base_->SetCapacity(capacity);
  data_ = base_->data();
}

bool RpcConnection::ReadIOBuffer::IncreaseCapacity() {
  if (GetCapacity() >= max_buffer_size_) {
    LOG(ERROR) << "Too large read data is pending: capacity=" << GetCapacity()
               << ", max_buffer_size=" << max_buffer_size_
               << ", read=" << GetSize();
    return false;
  }

  int new_capacity = GetCapacity() * kCapacityIncreaseFactor;
  if (new_capacity > max_buffer_size_)
    new_capacity = max_buffer_size_;
  SetCapacity(new_capacity);
  return true;
}

char* RpcConnection::ReadIOBuffer::StartOfBuffer() const {
  return base_->StartOfBuffer();
}

int RpcConnection::ReadIOBuffer::GetSize() const {
  return base_->offset();
}

void RpcConnection::ReadIOBuffer::DidRead(int bytes) {
  DCHECK_GE(RemainingCapacity(), bytes);
  base_->set_offset(base_->offset() + bytes);
  data_ = base_->data();
}

int RpcConnection::ReadIOBuffer::RemainingCapacity() const {
  return base_->RemainingCapacity();
}

void RpcConnection::ReadIOBuffer::DidConsume(int bytes) {
  int previous_size = GetSize();
  int unconsumed_size = previous_size - bytes;
  DCHECK_LE(0, unconsumed_size);
  if (unconsumed_size > 0) {
    // Move unconsumed data to the start of buffer.
    memmove(StartOfBuffer(), StartOfBuffer() + bytes, unconsumed_size);
  }
  base_->set_offset(unconsumed_size);
  data_ = base_->data();

  // If capacity is too big, reduce it.
  if (GetCapacity() > kMinimumBufSize &&
      GetCapacity() > previous_size * kCapacityIncreaseFactor) {
    int new_capacity = GetCapacity() / kCapacityIncreaseFactor;
    if (new_capacity < kMinimumBufSize)
      new_capacity = kMinimumBufSize;
    // realloc() within GrowableIOBuffer::SetCapacity() could move data even
    // when size is reduced. If unconsumed_size == 0, i.e. no data exists in
    // the buffer, free internal buffer first to guarantee no data move.
    if (!unconsumed_size)
      base_->SetCapacity(0);
    SetCapacity(new_capacity);
  }
}

RpcConnection::QueuedWriteIOBuffer::QueuedWriteIOBuffer()
    : total_size_(0),
      max_buffer_size_(kDefaultMaxBufferSize) {
}

RpcConnection::QueuedWriteIOBuffer::~QueuedWriteIOBuffer() {
  data_ = NULL;  // pending_data_ owns data_.
}

bool RpcConnection::QueuedWriteIOBuffer::IsEmpty() const {
  return pending_data_.empty();
}

bool RpcConnection::QueuedWriteIOBuffer::Append(const std::string& data) {
  if (data.empty())
    return true;

  if (total_size_ + static_cast<int>(data.size()) > max_buffer_size_) {
    LOG(ERROR) << "Too large write data is pending: size="
               << total_size_ + data.size()
               << ", max_buffer_size=" << max_buffer_size_;
    return false;
  }

  pending_data_.push(data);
  total_size_ += data.size();

  // If new data is the first pending data, updates data_.
  if (pending_data_.size() == 1)
    data_ = const_cast<char*>(pending_data_.front().data());
  return true;
}

void RpcConnection::QueuedWriteIOBuffer::DidConsume(int size) {
  DCHECK_GE(total_size_, size);
  DCHECK_GE(GetSizeToWrite(), size);
  if (size == 0)
    return;

  if (size < GetSizeToWrite()) {
    data_ += size;
  } else {  // size == GetSizeToWrite(). Updates data_ to next pending data.
    pending_data_.pop();
    data_ = IsEmpty() ? NULL : const_cast<char*>(pending_data_.front().data());
  }
  total_size_ -= size;
}

int RpcConnection::QueuedWriteIOBuffer::GetSizeToWrite() const {
  if (IsEmpty()) {
    DCHECK_EQ(0, total_size_);
    return 0;
  }
  DCHECK_GE(data_, pending_data_.front().data());
  int consumed = static_cast<int>(data_ - pending_data_.front().data());
  DCHECK_GT(static_cast<int>(pending_data_.front().size()), consumed);
  return pending_data_.front().size() - consumed;
}

RpcConnection::RpcConnection(int id, scoped_ptr<net::StreamSocket> socket)
    : id_(id),
      socket_(socket.Pass()),
      read_buf_(new ReadIOBuffer()),
      write_buf_(new QueuedWriteIOBuffer()),
      last_request_id_(0),
      weak_ptr_factory_(this) {
  net::IPEndPoint peer;
  socket_->GetPeerAddress(&peer);
  LOG(INFO) << peer.ToString();
}

RpcConnection::~RpcConnection() {
}

void RpcConnection::CallMethod(const google::protobuf::MethodDescriptor* method,
                               google::protobuf::RpcController*,
                               const google::protobuf::Message* request,
                               google::protobuf::Message* response,
                               google::protobuf::Closure* done) {
  rpc::RpcMessage message;
  message.set_id(last_request_id_++);
  message.set_type(rpc::RpcMessage::REQUEST);
  message.set_service(method->service()->full_name());
  message.set_method(method->name());
  message.set_request(request->SerializeAsString());
  request_id_to_response_map_[message.id()] = std::make_pair(response, done);

  char message_length[4];
  base::WriteBigEndian(message_length,
                       static_cast<uint32>(message.ByteSize()));
  std::string data(message_length, arraysize(message_length));
  data.append(message.SerializeAsString());
  write_buf()->Append(data);
  DoWriteLoop();
}

void RpcConnection::DoReadLoop() {
  int rv;
  do {
    // Increases read buffer size if necessary.
    if (read_buf_->RemainingCapacity() == 0 && !read_buf_->IncreaseCapacity()) {
      LOG(ERROR) << "Can not increase read buffer size";
      return;
    }

    rv = socket_->Read(read_buf_.get(),
                       read_buf_->RemainingCapacity(),
                       base::Bind(&RpcConnection::OnReadCompleted,
                                  weak_ptr_factory_.GetWeakPtr()));
    if (rv == net::ERR_IO_PENDING)
      return;
  } while (rv == net::OK);
}

void RpcConnection::OnReadCompleted(int rv) {
  if (HandleReadResult(rv) == net::OK)
    DoReadLoop();
}

int RpcConnection::HandleReadResult(int rv) {
  if (rv <= 0)
    return rv == 0 ? net::ERR_CONNECTION_CLOSED : rv;

  read_buf_->DidRead(rv);

  do {
    static const int kSizeOfUint32 = 4;
    if (read_buf_->GetSize() <= kSizeOfUint32)
      break;

    uint32 message_length;
    base::ReadBigEndian(read_buf_->StartOfBuffer(), &message_length);
    if (static_cast<uint32>(read_buf_->GetSize()) <
        (kSizeOfUint32 + message_length)) {
      break;
    }

    rpc::RpcMessage message;
    if (message.ParseFromArray(read_buf_->StartOfBuffer() + kSizeOfUint32,
                               message_length)) {
      read_buf_->DidConsume(kSizeOfUint32 + message_length);

      if (message.type() == RpcMessage::REQUEST)
        OnRequestMessage(message);
      else if (message.type() == RpcMessage::RESPONSE)
        OnReponseMessage(message);
      else
        NOTREACHED();
    }
  } while (true);

  return net::OK;
}

void RpcConnection::DoWriteLoop() {
  int rv = net::OK;
  while (rv == net::OK && write_buf_->GetSizeToWrite() > 0) {
    rv = socket_->Write(write_buf_.get(),
                        write_buf_->GetSizeToWrite(),
                        base::Bind(&RpcConnection::OnWriteCompleted,
                                   weak_ptr_factory_.GetWeakPtr()));
    if (rv == net::ERR_IO_PENDING || rv == net::OK)
      return;
    rv = HandleWriteResult(rv);
  }
}

void RpcConnection::OnWriteCompleted(int rv) {
  if (HandleWriteResult(rv) == net::OK)
    DoWriteLoop();
}

int RpcConnection::HandleWriteResult(int rv) {
  if (rv < 0)
    return rv;

  write_buf_->DidConsume(rv);
  return net::OK;
}

void RpcConnection::OnRequestMessage(const rpc::RpcMessage& message) {
  DCHECK_EQ(message.type(), RpcMessage::REQUEST);
  google::protobuf::Service* service =
      ServiceManager::GetInstance()->FindService(message.service());
  const google::protobuf::MethodDescriptor* method_descriptor =
      service->GetDescriptor()->FindMethodByName(message.method());
  if (method_descriptor == NULL) {
    LOG(ERROR) << "Unkonwn method: " << message.method();
    return;
  }

  // |parameters| will be deleted when OnServiceDone is called.
  rpc::RequestParameters* parameters = new rpc::RequestParameters(
      id_,
      service->GetRequestPrototype(method_descriptor).New(),
      service->GetResponsePrototype(method_descriptor).New(),
      message.id(),
      message.service(),
      message.method());
  parameters->request->ParseFromString(message.request());
  service->CallMethod(
      method_descriptor,
      NULL,
      parameters->request.get(),
      parameters->response.get(),
      google::protobuf::NewCallback(this, &RpcConnection::OnServiceDone,
                                    parameters));
}

void RpcConnection::OnReponseMessage(const rpc::RpcMessage& message) {
  DCHECK_EQ(message.type(), RpcMessage::RESPONSE);
  RequsetIdToResponseMap::const_iterator it =
      request_id_to_response_map_.find(message.id());
  if (it == request_id_to_response_map_.end()) {
    LOG(ERROR) << "Unknown request id received.";
    return;
  }

  Response response = it->second;
  DCHECK_EQ(message.type(), RpcMessage::RESPONSE);

  response.first->ParseFromString(message.response());
  if (response.second)
    response.second->Run();
  request_id_to_response_map_.erase(it);
}

void RpcConnection::OnServiceDone(RequestParameters* raw_parameters) {
  DCHECK_EQ(raw_parameters->connection_id, id_);
  scoped_ptr<rpc::RequestParameters> parameters(raw_parameters);
  rpc::RpcMessage message;
  message.set_id(parameters->request_id);
  message.set_type(rpc::RpcMessage::RESPONSE);
  message.set_service(parameters->service);
  message.set_method(parameters->method);
  message.set_response(parameters->response->SerializeAsString());

  char message_length[4];
  base::WriteBigEndian(message_length,
                       static_cast<uint32>(message.ByteSize()));
  std::string data(message_length, arraysize(message_length));
  data.append(message.SerializeAsString());
  write_buf_->Append(data);
  DoWriteLoop();
}

}  // namespace rpc
