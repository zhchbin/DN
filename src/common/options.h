// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  COMMON_OPTIONS_H_
#define  COMMON_OPTIONS_H_

namespace rpc {

extern const char kDefaultBindIP[];
extern const int kDefaultPort;

}  // namespace rpc

namespace switches {

extern const char kBindIP[];
extern const char kPort[];
extern const char kTargets[];
extern const char kSlaveAmount[];

extern const char kMaster[];

}  // namespace switches

#endif  // COMMON_OPTIONS_H_
