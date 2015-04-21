// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  COMMON_ASYNC_SUBPROCESS_H_
#define  COMMON_ASYNC_SUBPROCESS_H_

#include <string>

#include "base/message_loop/message_loop.h"
#include "third_party/ninja/src/exit_status.h"

namespace common {

class AsyncSubprocess
#if defined(OS_WIN)
    : public base::MessageLoopForIO::IOHandler {
#else
    : public base::MessageLoopForIO::Watcher {
#endif
 public:
  AsyncSubprocess();
  ~AsyncSubprocess() override;

  // Type definition for the subprocess exit callback.
  typedef base::Callback<void(AsyncSubprocess*)> ExitCallback;

  bool Start(const std::string& command, const ExitCallback& callback);
  bool Start(const std::string& command, const ExitCallback& callback,
             bool use_console);

  /// Returns ExitSuccess on successful process exit, ExitInterrupted if
  /// the process was interrupted, ExitFailure if it otherwise failed.
  ExitStatus Finish();

  bool Done() const;
  const std::string& GetOutput() const;

#if defined(OS_WIN)
  // Implementation of IOHandler on Windows.
  void OnIOCompleted(base::MessageLoopForIO::IOContext* context,
                     DWORD bytes_transfered,
                     DWORD error) override;
#elif defined(OS_POSIX)
  int fd() const { return fd_; }

  // Implementation of base::MessageLoopForIO::Watcher.
  void OnFileCanWriteWithoutBlocking(int fd) override {}
  void OnFileCanReadWithoutBlocking(int fd) override;
  void EnsureWatching();
#endif

 private:
  std::string buf_;
  bool use_console_;

#if defined(OS_WIN)
  /// Set up pipe_ as the parent-side pipe of the subprocess; return the
  /// other end of the pipe, usable in the child process.
  HANDLE SetupPipe();
  void OnPipeReady();

  HANDLE child_;
  HANDLE pipe_;
  base::MessageLoopForIO::IOContext* context_;
  bool is_reading_;
  char overlapped_buf_[4 << 10];
#elif defined(OS_POSIX)
  int fd_;
  pid_t pid_;
  base::MessageLoopForIO::FileDescriptorWatcher fd_watcher_;
  bool is_watching_;
#endif

  ExitCallback exit_callback_;

  DISALLOW_COPY_AND_ASSIGN(AsyncSubprocess);
};

}   // namespace common

#endif  // COMMON_ASYNC_SUBPROCESS_H_
