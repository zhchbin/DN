// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "rpc/rpc_server.h"

#include "base/bind.h"
#include "base/message_loop/message_loop_proxy.h"
#include "base/stl_util.h"
#include "base/tracked_objects.h"
#include "net/net_errors.h"
#include "net/server_socket.h"
#include "net/stream_socket.h"
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
  LOG(INFO) << read_buf->StartOfBuffer();

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
