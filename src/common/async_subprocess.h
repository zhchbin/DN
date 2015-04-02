// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  COMMON_ASYNC_SUBPROCESS_H_
#define  COMMON_ASYNC_SUBPROCESS_H_

#include <string>

#include "base/message_loop/message_loop.h"
#include "third_party/ninja/src/exit_status.h"

namespace common {

class AsyncSubprocess : public base::MessageLoopForIO::Watcher {
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
  int fd() const { return fd_; }

  // Implementation of base::MessageLoopForIO::Watcher.
  void OnFileCanWriteWithoutBlocking(int fd) override {}
  void OnFileCanReadWithoutBlocking(int fd) override;
  void EnsureWatching();

 private:
  std::string buf_;
  int fd_;
  pid_t pid_;
  bool use_console_;

  base::MessageLoopForIO::FileDescriptorWatcher fd_watcher_;
  bool is_watching_;

  ExitCallback exit_callback_;

  DISALLOW_COPY_AND_ASSIGN(AsyncSubprocess);
};

}   // namespace common

#endif  // COMMON_ASYNC_SUBPROCESS_H_
