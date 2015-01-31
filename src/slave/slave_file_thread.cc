// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "slave/slave_file_thread.h"

#include "base/bind.h"
#include "thread/ninja_thread.h"

namespace ninja {

SlaveFileThread::SlaveFileThread() {
  NinjaThread::SetDelegate(NinjaThread::FILE, this);
}

SlaveFileThread::~SlaveFileThread() {
  NinjaThread::SetDelegate(NinjaThread::FILE, NULL);
}

void SlaveFileThread::Init() {
  server_ = mg_create_server(NULL, NULL);
  mg_set_option(server_, "document_root", ".");      // Serve current directory
  mg_set_option(server_, "listening_port", "8080");  // Open port 8080
}

void SlaveFileThread::InitAsync() {
  NinjaThread::PostTask(
      NinjaThread::FILE,
      FROM_HERE,
      base::Bind(&SlaveFileThread::PoolMongooseServer, base::Unretained(this)));
}

void SlaveFileThread::CleanUp() {
  mg_destroy_server(&server_);
}

void SlaveFileThread::PoolMongooseServer() {
  mg_poll_server(server_, 1000);

  NinjaThread::PostTask(
      NinjaThread::FILE,
      FROM_HERE,
      base::Bind(&SlaveFileThread::PoolMongooseServer, base::Unretained(this)));
}

}  // namespace ninja
