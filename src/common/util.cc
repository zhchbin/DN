// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "common/util.h"

#include "base/sys_info.h"

namespace common {

int GuessParallelism() {
  switch (int processors = base::SysInfo::NumberOfProcessors()) {
  case 0:
  case 1:
    return 2;
  case 2:
    return 3;
  default:
    return processors + 2;
  }
}

}  // namespace common
