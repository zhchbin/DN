// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file and LICENSE file.

#ifndef NET_ADDRESS_LIST_H_
#define NET_ADDRESS_LIST_H_

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "net/ip_endpoint.h"
#include "net/net_util.h"

struct addrinfo;

namespace net {

class AddressList : private std::vector<IPEndPoint> {
 public:
  AddressList();
  ~AddressList();

  // Creates an address list for a single IP literal.
  explicit AddressList(const IPEndPoint& endpoint);

  static AddressList CreateFromIPAddress(const IPAddressNumber& address,
                                         uint16 port);

  static AddressList CreateFromIPAddressList(
      const IPAddressList& addresses,
      const std::string& canonical_name);

  // Copies the data from |head| and the chained list into an AddressList.
  static AddressList CreateFromAddrinfo(const struct addrinfo* head);

  // Returns a copy of |list| with port on each element set to |port|.
  static AddressList CopyWithPort(const AddressList& list, uint16 port);

  // TODO(szym): Remove all three. http://crbug.com/126134
  const std::string& canonical_name() const {
    return canonical_name_;
  }

  void set_canonical_name(const std::string& canonical_name) {
    canonical_name_ = canonical_name;
  }

  // Sets canonical name to the literal of the first IP address on the list.
  void SetDefaultCanonicalName();

  // Exposed methods from std::vector.
  using std::vector<IPEndPoint>::size;
  using std::vector<IPEndPoint>::empty;
  using std::vector<IPEndPoint>::clear;
  using std::vector<IPEndPoint>::reserve;
  using std::vector<IPEndPoint>::capacity;
  using std::vector<IPEndPoint>::operator[];
  using std::vector<IPEndPoint>::front;
  using std::vector<IPEndPoint>::back;
  using std::vector<IPEndPoint>::push_back;
  using std::vector<IPEndPoint>::insert;
  using std::vector<IPEndPoint>::erase;
  using std::vector<IPEndPoint>::iterator;
  using std::vector<IPEndPoint>::const_iterator;
  using std::vector<IPEndPoint>::begin;
  using std::vector<IPEndPoint>::end;
  using std::vector<IPEndPoint>::rbegin;
  using std::vector<IPEndPoint>::rend;

 private:
  std::string canonical_name_;
};

}  // namespace net

#endif  // NET_ADDRESS_LIST_H_
