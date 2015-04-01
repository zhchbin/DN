// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "slave/slave_file_thread.h"

#include "base/bind.h"
#include "common/options.h"
#include "thread/ninja_thread.h"

namespace {
static bool g_should_quit_pool = false;
}  // namespace

namespace slave {

// static
void SlaveFileThread::QuitPool() {
  DCHECK(NinjaThread::CurrentlyOn(NinjaThread::FILE));
  g_should_quit_pool = true;
}

SlaveFileThread::SlaveFileThread()
    : weak_factory_(this) {
  NinjaThread::SetDelegate(NinjaThread::FILE, this);
}

SlaveFileThread::~SlaveFileThread() {
  NinjaThread::SetDelegate(NinjaThread::FILE, NULL);
}

void SlaveFileThread::Init() {
  server_ = mg_create_server(NULL, NULL);
  mg_set_option(server_, "document_root", ".");      // Serve current directory

  CHECK(mg_set_option(server_, "listening_port",
                      options::kMongooseServerPort) == NULL)
      << "Failed to set listening_port option with value "
      << options::kMongooseServerPort;
}

void SlaveFileThread::InitAsync() {
  NinjaThread::PostTask(
      NinjaThread::FILE,
      FROM_HERE,
      base::Bind(&SlaveFileThread::PoolMongooseServer,
                 weak_factory_.GetWeakPtr()));
}

void SlaveFileThread::CleanUp() {
  mg_destroy_server(&server_);
}

void SlaveFileThread::PoolMongooseServer() {
  if (g_should_quit_pool)
    return;

  mg_poll_server(server_, 1000);
  NinjaThread::PostTask(
      NinjaThread::FILE,
      FROM_HERE,
      base::Bind(&SlaveFileThread::PoolMongooseServer,
                  weak_factory_.GetWeakPtr()));
}

}  // namespace slave
