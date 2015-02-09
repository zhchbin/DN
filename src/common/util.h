// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  COMMON_UTIL_H_
#define  COMMON_UTIL_H_

namespace common {

/// Choose a default value how many jobs can run in parallel.
int GuessParallelism();

}  // namespace common

#endif  // COMMON_UTIL_H_
