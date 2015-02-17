// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "common/util.h"
#include "master/curl_helper.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace master {

TEST(CurlHelperTest, GetWithMD5) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  base::FilePath license_file_path;
  PathService::Get(base::DIR_SOURCE_ROOT, &license_file_path);
  license_file_path = license_file_path.AppendASCII("LICENSE");

  base::FilePath file_path = temp_dir.path().AppendASCII("tmp_file");
  CurlHelper curl_helper;
  const std::string klicenseURL =
      "https://raw.githubusercontent.com/zhchbin/DN/master/LICENSE";
  EXPECT_EQ(curl_helper.Get(klicenseURL, file_path),
            common::GetMd5Digest(license_file_path));
}

}  // namespace master
