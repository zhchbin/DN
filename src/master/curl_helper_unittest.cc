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

  base::FilePath readme_file_path;
  PathService::Get(base::DIR_SOURCE_ROOT, &readme_file_path);
  readme_file_path = readme_file_path.AppendASCII("README.md");

  base::FilePath file_path = temp_dir.path().AppendASCII("tmp_file");
  CurlHelper curl_helper;
  const std::string kReadmeURL =
      "https://raw.githubusercontent.com/zhchbin/DN/master/README.md";
  EXPECT_EQ(curl_helper.Get(kReadmeURL, file_path),
            common::GetMd5Digest(readme_file_path));
}

}  // namespace master
