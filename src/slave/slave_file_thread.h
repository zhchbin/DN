// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  SLAVE_SLAVE_FILE_THREAD_H_
#define  SLAVE_SLAVE_FILE_THREAD_H_

#include "third_party/mongoose/mongoose.h"
#include "thread/ninja_thread_delegate.h"

namespace ninja {

class SlaveFileThread : public NinjaThreadDelegate {
 public:
  SlaveFileThread();
  ~SlaveFileThread() override;

  // NinjaThreadDelegate implementations.
  void Init() override;
  void InitAsync() override;
  void CleanUp() override;

  void PoolMongooseServer();

 private:
  mg_server* server_;
};

}  // namespace ninja

#endif  // SLAVE_SLAVE_FILE_THREAD_H_
