// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/run_loop.h"
#include "net/tcp_server_socket.h"
#include "ninja_thread_delegate.h"  // NOLINT
#include "ninja_thread_impl.h"      // NOLINT
#include "rpc/rpc_server.h"
#include "rpc/rpc_thread_delegate.h"

int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  base::AtExitManager exit_manager;

  scoped_ptr<base::MessageLoop> message_loop(new base::MessageLoop());
  scoped_ptr<NinjaThreadImpl> main_thread(
      new NinjaThreadImpl(NinjaThread::MAIN, base::MessageLoop::current()));

  scoped_ptr<NinjaThreadImpl> rpc_thread(new NinjaThreadImpl(NinjaThread::RPC));
  base::Thread::Options options(base::MessageLoop::TYPE_IO, 0);
  scoped_ptr<rpc::RpcThreadDelegate> rpc_thread_delegate(
      new rpc::RpcThreadDelegate());

  NinjaThread::SetDelegate(NinjaThread::RPC, rpc_thread_delegate.get());
  rpc_thread->StartWithOptions(options);

  base::RunLoop run_loop;
  run_loop.Run();
  rpc_thread->Stop();

  return 0;
}
