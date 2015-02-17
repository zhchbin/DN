// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  MASTER_CURL_HELPER_H_
#define  MASTER_CURL_HELPER_H_

#include <string>

#include "base/files/file.h"
#include "base/md5.h"
#include "curl/curl.h"

namespace base {
class FilePath;
}  // namespace base

namespace master {

// Curl helper, with md5 check sum.
class CurlHelper {
 public:
  static size_t CurlOptWriteFunction(void* ptr, size_t size, size_t count,
                                     CurlHelper* curl_helper);

  CurlHelper();
  ~CurlHelper();

  // Performs HTTP get to download the |url| to |filename|. Returns md5 digest
  // of the file if success, otherwise returns an empty string.
  std::string Get(const std::string& url, const base::FilePath& filename);

 private:
  size_t WriteData(void* ptr, size_t size, size_t count);

  CURL* curl_;
  base::MD5Context md5_context_;
  base::File file_;

  DISALLOW_COPY_AND_ASSIGN(CurlHelper);
};

}  // namespace master

#endif  // MASTER_CURL_HELPER_H_
