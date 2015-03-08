// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "master/webui_thread.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "master/master_main_runner.h"
#include "thread/ninja_thread.h"

namespace master {

// static
int WebUIThread::EventHandler(mg_connection* conn, mg_event ev) {
  WebUIThread* webui = static_cast<WebUIThread*>(conn->server_param);
  switch (ev) {
    case MG_AUTH:
      return MG_TRUE;
    case MG_REQUEST:
      if (strcmp(conn->uri, "/api/start") == 0) {
        webui->HandleStart(conn);
        return MG_TRUE;
      } else if (strcmp(conn->uri, "/api/initial_status") == 0) {
        webui->HandleGetInitialStatus(conn);
        return MG_TRUE;
      } else  if (strcmp(conn->uri, "/api/result") == 0) {
        webui->HandleGetResult(conn);
        return MG_TRUE;
      }

      return MG_FALSE;
    default:
      return MG_FALSE;
  }
}

WebUIThread::WebUIThread(MasterMainRunner* main_runner)
    : master_main_runner_(main_runner),
      weak_factory_(this) {
  NinjaThread::SetDelegate(NinjaThread::FILE, this);
}

WebUIThread::~WebUIThread() {
  NinjaThread::SetDelegate(NinjaThread::FILE, NULL);
}

void WebUIThread::Init() {
  server_ = mg_create_server(this, &WebUIThread::EventHandler);
  base::FilePath path;
  if (PathService::Get(base::DIR_EXE, &path)) {
    path = path.AppendASCII("webui");
    mg_set_option(server_, "document_root", path.AsUTF8Unsafe().c_str());
  }

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

void WebUIThread::HandleStart(mg_connection* conn) {
  NinjaThread::PostTask(
      NinjaThread::MAIN,
      FROM_HERE,
      base::Bind(&MasterMainRunner::StartBuild, master_main_runner_));
  mg_printf_data(conn, "{ \"result\": \"OK\" }");
}

void WebUIThread::HandleGetInitialStatus(mg_connection* conn) {
  mg_printf_data(conn, initial_status_.c_str());
}

void WebUIThread::HandleGetResult(mg_connection* conn) {
  mg_printf_data(conn, "[");
  for (size_t i = 0; i < command_results_.size(); ++i) {
    mg_printf_data(conn, command_results_[i].c_str());
    if (i != command_results_.size() - 1)
      mg_printf_data(conn, ",");
  }
  command_results_.clear();

  mg_printf_data(conn, "]");
}

}  // namespace master
