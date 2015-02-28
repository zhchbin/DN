// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "master/webui_thread.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "thread/ninja_thread.h"

namespace master {

// static
int WebUIThread::EventHandler(mg_connection* conn, mg_event ev) {
  switch (ev) {
    case MG_AUTH:
      return MG_TRUE;
    case MG_REQUEST:
      if (!strcmp(conn->uri, "/api/sum")) {
        static_cast<WebUIThread*>(conn->server_param)->HandleSum(conn);
        return MG_TRUE;
      }

      return MG_FALSE;
    default: return MG_FALSE;
  }
}

WebUIThread::WebUIThread() : weak_factory_(this) {
  NinjaThread::SetDelegate(NinjaThread::FILE, this);
}

WebUIThread::~WebUIThread() {
  NinjaThread::SetDelegate(NinjaThread::FILE, NULL);
}

void WebUIThread::Init() {
  server_ = mg_create_server(this, &WebUIThread::EventHandler);
  base::FilePath path;
  if (PathService::Get(base::DIR_EXE, &path))
    mg_set_option(server_, "document_root", path.AsUTF8Unsafe().c_str());

  const int kMinListeningPort = 9000;
  for (size_t i = kMinListeningPort; i < kMinListeningPort + 10; ++i) {
    if (mg_set_option(server_, "listening_port", base::UintToString(i).c_str())
          == NULL) {
      LOG(INFO) << "WebUI: " << "http://127.0.0.1:" << i;
      break;
    }
  }
}

void WebUIThread::InitAsync() {
  NinjaThread::PostTask(
      NinjaThread::FILE,
      FROM_HERE,
      base::Bind(&WebUIThread::PoolMongooseServer,
                 weak_factory_.GetWeakPtr()));
}

void WebUIThread::CleanUp() {
  mg_destroy_server(&server_);
}

void WebUIThread::PoolMongooseServer() {
  mg_poll_server(server_, 1000);
  NinjaThread::PostTask(
      NinjaThread::FILE,
      FROM_HERE,
      base::Bind(&WebUIThread::PoolMongooseServer,
                 weak_factory_.GetWeakPtr()));
}

void WebUIThread::HandleSum(mg_connection* conn) {
  char n1[100], n2[100];

  // Get form variables
  mg_get_var(conn, "n1", n1, sizeof(n1));
  mg_get_var(conn, "n2", n2, sizeof(n2));

  mg_printf_data(conn, "{ \"result\": %lf }",
                 strtod(n1, NULL) + strtod(n2, NULL));
}

}  // namespace master
