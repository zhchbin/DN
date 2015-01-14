// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file and LICENSE file.

#include "net/server_socket.h"

#include <vector>

#include "net/ip_endpoint.h"
#include "net/net_errors.h"
#include "net/net_util.h"

namespace net {

ServerSocket::ServerSocket() {
}

ServerSocket::~ServerSocket() {
}

int ServerSocket::ListenWithAddressAndPort(const std::string& address_string,
                                           uint16 port,
                                           int backlog) {
  IPAddressNumber address_number;
  if (!ParseIPLiteralToNumber(address_string, &address_number)) {
    return ERR_ADDRESS_INVALID;
  }

  return Listen(IPEndPoint(address_number, port), backlog);
}

}  // namespace net
