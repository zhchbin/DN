// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  SLAVE_SLAVE_FILE_THREAD_H_
#define  SLAVE_SLAVE_FILE_THREAD_H_

#include "base/memory/weak_ptr.h"
#include "third_party/mongoose/mongoose.h"
#include "thread/ninja_thread_delegate.h"

namespace slave {

class SlaveFileThread : public NinjaThreadDelegate {
 public:
  static void QuitPool();

  SlaveFileThread();
  ~SlaveFileThread() override;

  // NinjaThreadDelegate implementations.
  void Init() override;
  void InitAsync() override;
  void CleanUp() override;

  void PoolMongooseServer();

 private:
  base::WeakPtrFactory<SlaveFileThread> weak_factory_;
  mg_server* server_;
};

}  // namespace slave

#endif  // SLAVE_SLAVE_FILE_THREAD_H_
