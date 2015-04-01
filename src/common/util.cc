// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "common/util.h"

#include "base/files/file.h"
#include "base/hash.h"
#include "base/md5.h"
#include "base/strings/string_util.h"
#include "base/sys_info.h"
#include "third_party/ninja/src/graph.h"

namespace {
const int kMd5DigestBufferSize = 512 * 1024;  // 512 kB.
}  // namespace

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

std::string GetMd5Digest(const base::FilePath& file_path) {
  base::File file(file_path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!file.IsValid())
    return std::string();

  base::MD5Context context;
  base::MD5Init(&context);

  int64 offset = 0;
  scoped_ptr<char[]> buffer(new char[kMd5DigestBufferSize]);
  while (true) {
    int result = file.Read(offset, buffer.get(), kMd5DigestBufferSize);
    if (result < 0) {
      // Found an error.
      return std::string();
    }

    if (result == 0) {
      // End of file.
      break;
    }

    offset += result;
    base::MD5Update(&context, base::StringPiece(buffer.get(), result));
  }

  base::MD5Digest digest;
  base::MD5Final(&digest, &context);
  return MD5DigestToBase16(digest);
}

uint32 HashEdge(const Edge* edge) {
  static const char kWhitespace[] = " ";
  std::string rule_and_targets = edge->rule().name();
  if (!edge->outputs_.empty())
    rule_and_targets += kWhitespace;
  for (size_t i = 0; i < edge->outputs_.size(); ++i) {
    rule_and_targets += edge->outputs_[i]->path();
    if (i != edge->outputs_.size() - 1)
      rule_and_targets += kWhitespace;
  }

  return base::Hash(base::StringToLowerASCII(rule_and_targets));
}

}  // namespace common
