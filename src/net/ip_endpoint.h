// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file and LICENSE file.

#ifndef NET_IP_ENDPOINT_H_
#define NET_IP_ENDPOINT_H_

#include <string>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "net/address_family.h"
#include "net/net_util.h"

struct sockaddr;

namespace net {

// An IPEndPoint represents the address of a transport endpoint:
//  * IP address (either v4 or v6)
//  * Port
class IPEndPoint {
 public:
  IPEndPoint();
  ~IPEndPoint();
  IPEndPoint(const IPAddressNumber& address, uint16 port);
  IPEndPoint(const IPEndPoint& endpoint);

  const IPAddressNumber& address() const { return address_; }
  uint16 port() const { return port_; }

  // Returns AddressFamily of the address.
  AddressFamily GetFamily() const;

  // Returns the sockaddr family of the address, AF_INET or AF_INET6.
  int GetSockAddrFamily() const;

  // Convert to a provided sockaddr struct.
  // |address| is the sockaddr to copy into.  Should be at least
  //    sizeof(struct sockaddr_storage) bytes.
  // |address_length| is an input/output parameter.  On input, it is the
  //    size of data in |address| available.  On output, it is the size of
  //    the address that was copied into |address|.
  // Returns true on success, false on failure.
  bool ToSockAddr(struct sockaddr* address, socklen_t* address_length) const
      WARN_UNUSED_RESULT;

  // Convert from a sockaddr struct.
  // |address| is the address.
  // |address_length| is the length of |address|.
  // Returns true on success, false on failure.
  bool FromSockAddr(const struct sockaddr* address, socklen_t address_length)
      WARN_UNUSED_RESULT;

  // Returns value as a string (e.g. "127.0.0.1:80"). Returns empty
  // string if the address is invalid, and cannot not be converted to a
  // string.
  std::string ToString() const;

  // As above, but without port.
  std::string ToStringWithoutPort() const;

  bool operator<(const IPEndPoint& that) const;
  bool operator==(const IPEndPoint& that) const;

 private:
  IPAddressNumber address_;
  uint16 port_;
};

}  // namespace net

#endif  // NET_IP_ENDPOINT_H_
