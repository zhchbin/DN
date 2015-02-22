// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  MASTER_WEBUI_THREAD_H_
#define  MASTER_WEBUI_THREAD_H_

#include "base/memory/weak_ptr.h"
#include "third_party/mongoose/mongoose.h"
#include "thread/ninja_thread_delegate.h"

namespace master {

class WebUIThread : public NinjaThreadDelegate {
 public:
  static int EventHandler(mg_connection* conn, mg_event ev);

  WebUIThread();
  ~WebUIThread() override;

  // NinjaThreadDelegate implementations.
  void Init() override;
  void InitAsync() override;
  void CleanUp() override;

  void PoolMongooseServer();

 private:
  void HandleSum(mg_connection* conn);

  mg_server* server_;
  base::WeakPtrFactory<WebUIThread> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(WebUIThread);
};

}  // namespace master

#endif  // MASTER_WEBUI_THREAD_H_
