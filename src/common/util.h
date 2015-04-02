// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  COMMON_UTIL_H_
#define  COMMON_UTIL_H_

#include <string>

#include "base/basictypes.h"

namespace base {
class FilePath;
}  // namespace base

struct Edge;

namespace common {

/// Choose a default value how many jobs can run in parallel.
int GuessParallelism();

std::string GetMd5Digest(const base::FilePath& file_path);

uint32 HashEdge(const Edge* edge);

int SetNonBlocking(int fd);

}  // namespace common

#endif  // COMMON_UTIL_H_
