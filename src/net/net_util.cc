// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file and LICENSE file.

#include "net/net_util.h"

#include <string>
#include <vector>

#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#include <iphlpapi.h>
#include <winsock2.h>
#include <ws2bth.h>
#pragma comment(lib, "iphlpapi.lib")
#elif defined(OS_POSIX)
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#endif  // defined(OS_POSIX)

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/sys_byteorder.h"

namespace net {

SockaddrStorage::SockaddrStorage(const SockaddrStorage& other)
    : addr_len(other.addr_len),
      addr(reinterpret_cast<struct sockaddr*>(&addr_storage)) {
  memcpy(addr, other.addr, addr_len);
}

void SockaddrStorage::operator=(const SockaddrStorage& other) {
  addr_len = other.addr_len;
  // addr is already set to &this->addr_storage by default ctor.
  memcpy(addr, other.addr, addr_len);
}

// Extracts the address and port portions of a sockaddr.
bool GetIPAddressFromSockAddr(const struct sockaddr* sock_addr,
                              socklen_t sock_addr_len,
                              const uint8** address,
                              size_t* address_len,
                              uint16* port) {
  if (sock_addr->sa_family == AF_INET) {
    if (sock_addr_len < static_cast<socklen_t>(sizeof(struct sockaddr_in)))
      return false;
    const struct sockaddr_in* addr =
        reinterpret_cast<const struct sockaddr_in*>(sock_addr);
    *address = reinterpret_cast<const uint8*>(&addr->sin_addr);
    *address_len = kIPv4AddressSize;
    if (port)
      *port = base::NetToHost16(addr->sin_port);
    return true;
  }

  if (sock_addr->sa_family == AF_INET6) {
    if (sock_addr_len < static_cast<socklen_t>(sizeof(struct sockaddr_in6)))
      return false;
    const struct sockaddr_in6* addr =
        reinterpret_cast<const struct sockaddr_in6*>(sock_addr);
    *address = reinterpret_cast<const uint8*>(&addr->sin6_addr);
    *address_len = kIPv6AddressSize;
    if (port)
      *port = base::NetToHost16(addr->sin6_port);
    return true;
  }

#if defined(OS_WIN)
  if (sock_addr->sa_family == AF_BTH) {
    if (sock_addr_len < static_cast<socklen_t>(sizeof(SOCKADDR_BTH)))
      return false;
    const SOCKADDR_BTH* addr =
        reinterpret_cast<const SOCKADDR_BTH*>(sock_addr);
    *address = reinterpret_cast<const uint8*>(&addr->btAddr);
    *address_len = kBluetoothAddressSize;
    if (port)
      *port = static_cast<uint16>(addr->port);
    return true;
  }
#endif

  return false;  // Unrecognized |sa_family|.
}

AddressFamily GetAddressFamily(const IPAddressNumber& address) {
  switch (address.size()) {
    case kIPv4AddressSize:
      return ADDRESS_FAMILY_IPV4;
    case kIPv6AddressSize:
      return ADDRESS_FAMILY_IPV6;
    default:
      return ADDRESS_FAMILY_UNSPECIFIED;
  }
}

// Set socket to non-blocking mode
int SetNonBlocking(int fd) {
#if defined(OS_WIN)
  uint64 no_block = 1;
  return ioctlsocket(fd, FIONBIO, &no_block);
#elif defined(OS_POSIX)
  int flags = fcntl(fd, F_GETFL, 0);
  if (-1 == flags)
    return flags;
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#endif
}

int ConvertAddressFamily(AddressFamily address_family) {
  switch (address_family) {
    case ADDRESS_FAMILY_UNSPECIFIED:
      return AF_UNSPEC;
    case ADDRESS_FAMILY_IPV4:
      return AF_INET;
    case ADDRESS_FAMILY_IPV6:
      return AF_INET6;
  }
  NOTREACHED();
  return AF_UNSPEC;
}

bool ParseIPLiteralToNumber(const std::string& ip_literal,
                            IPAddressNumber* ip_number) {
  ip_number->resize(4);
  std::vector<std::string> components;
  base::SplitString(ip_literal, '.', &components);
  if (components.size() != 4)
    return false;
  for (size_t i = 0; i < components.size(); ++i) {
    uint32 r;
    if (!base::StringToUint(components[i], &r) || r > 0xFF)
      return false;
    (*ip_number)[i] = r;
  }

  return true;
}

}  // namespace net
