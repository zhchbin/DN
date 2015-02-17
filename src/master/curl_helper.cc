// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "master/curl_helper.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"

namespace master {

// static
size_t CurlHelper::CurlOptWriteFunction(void* ptr, size_t size, size_t count,
                                        CurlHelper* curl_helper) {
  return curl_helper->WriteData(ptr, size, count);
}

CurlHelper::CurlHelper() : curl_(curl_easy_init()) {
}

CurlHelper::~CurlHelper() {
  curl_easy_cleanup(curl_);
}

std::string CurlHelper::Get(const std::string& url,
                            const base::FilePath& filename) {
  base::MD5Init(&md5_context_);
  CHECK(base::CreateDirectory(filename.DirName()));
  file_.InitializeUnsafe(filename,
                         base::File::FLAG_CREATE | base::File::FLAG_WRITE);
  if (!file_.IsValid()) {
    LOG(ERROR) << filename.value();
    return "";
  }

  curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION,
                   CurlHelper::CurlOptWriteFunction);
  curl_easy_setopt(curl_, CURLOPT_WRITEDATA, this);
  if (curl_easy_perform(curl_) != CURLE_OK)
    return "";

  file_.Unlock();
  file_.Close();
  base::MD5Digest digest;
  base::MD5Final(&digest, &md5_context_);
  return MD5DigestToBase16(digest);
}

size_t CurlHelper::WriteData(void* ptr, size_t size, size_t count) {
  base::StringPiece data(reinterpret_cast<char*>(ptr), size * count);
  base::MD5Update(&md5_context_, data);
  return file_.WriteAtCurrentPos(data.data(), data.size());
}

}  // namespace master
