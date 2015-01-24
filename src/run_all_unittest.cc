// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "base/at_exit.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gmock/include/gmock/gmock.h"

int main(int argc, char **argv) {
  base::AtExitManager exit_manager;
  ::testing::InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}
