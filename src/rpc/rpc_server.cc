// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "rpc/rpc_server.h"

#include "base/big_endian.h"
#include "base/bind.h"
#include "base/message_loop/message_loop_proxy.h"
#include "base/stl_util.h"
#include "base/tracked_objects.h"
#include "google/protobuf/message.h"
#include "net/net_errors.h"
#include "net/server_socket.h"
#include "net/stream_socket.h"
#include "proto/rpc_message.pb.h"
#include "rpc/rpc_connection.h"

namespace rpc {

RpcServer::RpcServer(scoped_ptr<net::ServerSocket> server_socket)
    : server_socket_(server_socket.Pass()),
      last_id_(0),
      weak_ptr_factory_(this) {
  DCHECK(server_socket_);

  // Start accepting connections in next run loop in case when delegate is not
  // ready to get callbacks.
  base::MessageLoopProxy::current()->PostTask(
      FROM_HERE,
      base::Bind(&RpcServer::DoAcceptLoop, weak_ptr_factory_.GetWeakPtr()));
}

RpcServer::~RpcServer() {
  STLDeleteContainerPairSecondPointers(
      id_to_connection_.begin(), id_to_connection_.end());
  STLDeleteContainerPairSecondPointers(
      service_map_.begin(), service_map_.end());
}

void RpcServer::Close(int connection_id) {
  RpcConnection* connection = FindConnection(connection_id);
  if (connection == NULL)
    return;
  id_to_connection_.erase(connection_id);

  // The call stack might have callbacks which still have the pointer of
  // connection. Instead of referencing connection with ID all the time,
  // destroys the connection in next run loop to make sure any pending
  // callbacks in the call stack return.
  base::MessageLoopProxy::current()->DeleteSoon(FROM_HERE, connection);
}

void RpcServer::DoAcceptLoop() {
  int rv;
  do {
    rv = server_socket_->Accept(&accepted_socket_,
                                base::Bind(&RpcServer::OnAcceptCompleted,
                                           weak_ptr_factory_.GetWeakPtr()));
    if (rv == net::ERR_IO_PENDING)
      return;
    rv = HandleAcceptResult(rv);
  } while (rv == net::OK);
}

void RpcServer::OnAcceptCompleted(int rv) {
  if (HandleAcceptResult(rv) == net::OK)
    DoAcceptLoop();
}

int RpcServer::HandleAcceptResult(int rv) {
  if (rv < 0) {
    LOG(ERROR) << "Accept error: rv=" << rv;
    return rv;
  }

  RpcConnection* connection =
      new RpcConnection(++last_id_, accepted_socket_.Pass());
  id_to_connection_[connection->id()] = connection;
  if (!HasConnectionClosed(connection))
    DoReadLoop(connection);
  return net::OK;
}

void RpcServer::DoReadLoop(RpcConnection* connection) {
  int rv;
  do {
    RpcConnection::ReadIOBuffer* read_buf = connection->read_buf();
    // Increases read buffer size if necessary.
    if (read_buf->RemainingCapacity() == 0 && !read_buf->IncreaseCapacity()) {
      Close(connection->id());
      return;
    }

    rv = connection->socket()->Read(
        read_buf,
        read_buf->RemainingCapacity(),
        base::Bind(&RpcServer::OnReadCompleted,
                   weak_ptr_factory_.GetWeakPtr(), connection->id()));
    if (rv == net::ERR_IO_PENDING)
      return;
    rv = HandleReadResult(connection, rv);
  } while (rv == net::OK);
}

void RpcServer::OnReadCompleted(int connection_id, int rv) {
  RpcConnection* connection = FindConnection(connection_id);
  if (!connection)  // It might be closed right before by write error.
    return;

  if (HandleReadResult(connection, rv) == net::OK)
    DoReadLoop(connection);
}

int RpcServer::HandleReadResult(RpcConnection* connection, int rv) {
  if (rv <= 0) {
    Close(connection->id());
    return rv == 0 ? net::ERR_CONNECTION_CLOSED : rv;
  }
  RpcConnection::ReadIOBuffer* read_buf = connection->read_buf();
  read_buf->DidRead(rv);

  do {
    static const int kSizeOfUint32 = 4;
    if (read_buf->GetSize() <= kSizeOfUint32)
      break;

    uint32 message_length;
    base::ReadBigEndian(read_buf->StartOfBuffer(), &message_length);
    if (static_cast<uint32>(read_buf->GetSize()) <
        (kSizeOfUint32 + message_length)) {
      break;
    }

    rpc::RpcMessage message;
    if (message.ParseFromArray(read_buf->StartOfBuffer() + kSizeOfUint32,
                               message_length)) {
      read_buf->DidConsume(kSizeOfUint32 + message_length);
      ServiceMap::const_iterator it = service_map_.find(message.service());
      if (it == service_map_.end()) {
        LOG(ERROR) << "Unknown service: " << message.service();
        break;
      }

      pb::Service* service = it->second;
      const pb::MethodDescriptor* method_descriptor =
          service->GetDescriptor()->FindMethodByName(message.method());
      if (method_descriptor == NULL) {
        LOG(ERROR) << "Unkonwn method: " << message.method();
        break;
      }

      // |parameters| will be deleted when OnServiceDone is called.
      RequestParameters* parameters = new RequestParameters(
          connection->id(),
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
          pb::NewCallback(this, &RpcServer::OnServiceDone, parameters));
    }
  } while (true);

  return net::OK;
}

void RpcServer::DoWriteLoop(RpcConnection* connection) {
  int rv = net::OK;
  RpcConnection::QueuedWriteIOBuffer* write_buf = connection->write_buf();
  while (rv == net::OK && write_buf->GetSizeToWrite() > 0) {
    rv = connection->socket()->Write(
        write_buf,
        write_buf->GetSizeToWrite(),
        base::Bind(&RpcServer::OnWriteCompleted,
                   weak_ptr_factory_.GetWeakPtr(), connection->id()));
    if (rv == net::ERR_IO_PENDING || rv == net::OK)
      return;
    rv = HandleWriteResult(connection, rv);
  }
}

void RpcServer::OnWriteCompleted(int connection_id, int rv) {
  RpcConnection* connection = FindConnection(connection_id);
  if (!connection)  // It might be closed right before by read error.
    return;

  if (HandleWriteResult(connection, rv) == net::OK)
    DoWriteLoop(connection);
}

int RpcServer::HandleWriteResult(RpcConnection* connection, int rv) {
  if (rv < 0) {
    Close(connection->id());
    return rv;
  }

  connection->write_buf()->DidConsume(rv);
  return net::OK;
}

void RpcServer::OnServiceDone(RequestParameters* raw_parameters) {
  scoped_ptr<RequestParameters> parameters(raw_parameters);
  RpcConnection* connection = FindConnection(parameters->connection_id);
  if (connection == NULL)
    return;

  rpc::RpcMessage message;
  message.set_id(parameters->request_id);
  message.set_type(RpcMessage::RESPONSE);
  message.set_service(parameters->service);
  message.set_method(parameters->method);
  message.set_response(parameters->response->SerializeAsString());
  char message_length[4];
  base::WriteBigEndian(message_length,
                       static_cast<uint32>(message.ByteSize()));
  std::string data(message_length, arraysize(message_length));
  data.append(message.SerializeAsString());
  connection->write_buf()->Append(data);
  DoWriteLoop(connection);
}

RpcConnection* RpcServer::FindConnection(int connection_id) {
  IdToConnectionMap::iterator it = id_to_connection_.find(connection_id);
  if (it == id_to_connection_.end())
    return NULL;

  return it->second;
}

bool RpcServer::HasConnectionClosed(RpcConnection* connection) {
  return FindConnection(connection->id()) != connection;
}

}  // namespace rpc
