// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "common/util.h"
#include "master/curl_helper.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace master {

#if defined(OS_LINUX)
// Our precompiled libcurl.lib does not support SSL yet.
#define MAYBE_GetWithMD5 GetWithMD5
#else
#define MAYBE_GetWithMD5 DISABLED_GetWithMD5
#endif

TEST(CurlHelperTest, MAYBE_GetWithMD5) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  base::FilePath license_file_path;
  PathService::Get(base::DIR_SOURCE_ROOT, &license_file_path);
  license_file_path = license_file_path.AppendASCII("LICENSE");

  CurlHelper curl_helper;
  const std::string klicenseURL =
      "https://raw.githubusercontent.com/zhchbin/DN/master/LICENSE";
  EXPECT_EQ(common::GetMd5Digest(license_file_path),
            curl_helper.Get(klicenseURL, temp_dir.path().AppendASCII("file")));
}

}  // namespace master
