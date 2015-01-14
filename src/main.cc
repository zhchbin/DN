// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "net/tcp_server_socket.h"
#include "ninja_thread_delegate.h"  // NOLINT
#include "ninja_thread_impl.h"      // NOLINT
#include "rpc/rpc_client_main.h"
#include "rpc/rpc_options.h"
#include "rpc/rpc_server.h"
#include "rpc/rpc_server_main.h"

namespace {
const int kMinPort = 1024;
const int kMaxPort = 65535;
}  // namespace

int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  base::AtExitManager exit_manager;

  scoped_ptr<base::MessageLoop> message_loop(new base::MessageLoop());
  scoped_ptr<NinjaThreadImpl> main_thread(
      new NinjaThreadImpl(NinjaThread::MAIN, base::MessageLoop::current()));

  scoped_ptr<NinjaThreadImpl> rpc_thread(new NinjaThreadImpl(NinjaThread::RPC));

  scoped_ptr<NinjaThreadDelegate> rpc_thread_delegate;
  const base::CommandLine* command_line =
    base::CommandLine::ForCurrentProcess();
  uint port = rpc::kDefaultPort;
  if (command_line->HasSwitch(switches::kPort)) {
    std::string port_str = command_line->GetSwitchValueASCII(switches::kPort);
    DCHECK(!port_str.empty());
    base::StringToUint(port_str, &port);
    CHECK(port >= kMinPort && port <= kMaxPort)
        << "Port should be in range [" << kMinPort << ", " << kMaxPort << "].";
  }

  // Setup whether rpc thread running as client or server.
  if (command_line->HasSwitch(switches::kServerIP)) {
    LOG(INFO) << "Running as rpc client.";
    std::string server_ip;
    server_ip = command_line->GetSwitchValueASCII(switches::kServerIP);
    DCHECK(!server_ip.empty());
    rpc_thread_delegate.reset(new rpc::RpcClientMain(server_ip, port));
  } else {
    LOG(INFO) << "Running as rpc server.";
    std::string bind_ip = rpc::kDefaultBindIP;
    if (command_line->HasSwitch(switches::kBindIP)) {
      bind_ip = command_line->GetSwitchValueASCII(switches::kBindIP);
      DCHECK(!bind_ip.empty());
    }
    rpc_thread_delegate.reset(new rpc::RpcServerMain(bind_ip, port));
  }

  NinjaThread::SetDelegate(NinjaThread::RPC, rpc_thread_delegate.get());
  base::Thread::Options options(base::MessageLoop::TYPE_IO, 0);
  rpc_thread->StartWithOptions(options);
  base::RunLoop run_loop;
  run_loop.Run();
  rpc_thread->Stop();

  return 0;
}
