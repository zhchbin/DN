// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "rpc/rpc_socket_client.h"

#include "net/address_list.h"
#include "net/net_errors.h"
#include "net/net_util.h"
#include "net/tcp_client_socket.h"

namespace rpc {

RpcSocketClient::RpcSocketClient(const std::string& server_ip, uint16 port)
    : server_ip_(server_ip),
      port_(port) {
}

RpcSocketClient::~RpcSocketClient() {
}

void RpcSocketClient::Connect() {
  Connect(net::CompletionCallback());
}

void RpcSocketClient::Connect(const net::CompletionCallback& callback) {
  net::IPAddressNumber ip_number;
  net::ParseIPLiteralToNumber(server_ip_, &ip_number);
  net::AddressList address_list;
  address_list = net::AddressList::CreateFromIPAddress(ip_number, port_);
  socket_.reset(new net::TCPClientSocket(address_list));
  int result = socket_->Connect(
      base::Bind(&RpcSocketClient::OnConnectComplete,
                 base::Unretained(this),
                 callback));
  if (result != net::ERR_IO_PENDING)
    OnConnectComplete(callback, result);
}

void RpcSocketClient::Disconnect() {
  rpc_connection_->socket()->Disconnect();
}

void RpcSocketClient::OnClose(RpcConnection* connection) {
  DCHECK(connection == rpc_connection_.get());
}

RpcConnection* RpcSocketClient::connection() {
  return rpc_connection_.get();
}

void RpcSocketClient::OnConnectComplete(const net::CompletionCallback& callback,
                                        int result) {
  CHECK(result == net::OK) << "Can't not connect to master.";
  static const int kOneMegabyte = 1024 * 1024;
  socket_->SetSendBufferSize(kOneMegabyte);
  rpc_connection_.reset(new RpcConnection(0, socket_.Pass(), this));
  rpc_connection_->DoReadLoop();
  if (!callback.is_null())
    callback.Run(result);
}

}   // namespace rpc
