// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "common/async_subprocess.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "common/util.h"
#include "third_party/ninja/src/util.h"

namespace common {

AsyncSubprocess::AsyncSubprocess()
    : fd_(-1),
      pid_(-1),
      use_console_(false),
      is_watching_(false) {
}

AsyncSubprocess::~AsyncSubprocess() {
  if (fd_ >= 0)
    close(fd_);
  // Reap child if forgotten.
  if (pid_ != -1)
    Finish();
}

bool AsyncSubprocess::Start(const std::string& command,
                            const ExitCallback& callback) {
  return Start(command, callback, false);
}

bool AsyncSubprocess::Start(const std::string& command,
                            const ExitCallback& callback,
                            bool use_console) {
  use_console_ = use_console;
  int output_pipe[2];
  if (pipe(output_pipe) < 0) {
    LOG(ERROR) << "pipe: " << strerror(errno);
    return false;
  }
  fd_ = output_pipe[0];
  SetCloseOnExec(fd_);
  common::SetNonBlocking(fd_);
  exit_callback_ = callback;
  EnsureWatching();

  pid_ = fork();
  if (pid_ < 0) {
    LOG(ERROR) << "fork: " << strerror(errno);
    return false;
  }

  if (pid_ == 0) {
    close(output_pipe[0]);

    // Track which fd we use to report errors on.
    int error_pipe = output_pipe[1];
    do {
      if (!use_console_) {
        // Put the child in its own session and process group. It will be
        // detached from the current terminal and ctrl-c won't reach it.
        // Since this process was just forked, it is not a process group leader
        // and setsid() will succeed.
        if (setsid() < 0)
          break;

        // Open /dev/null over stdin.
        int devnull = open("/dev/null", O_RDONLY);
        if (devnull < 0)
          break;
        if (dup2(devnull, 0) < 0)
          break;
        close(devnull);

        if (dup2(output_pipe[1], 1) < 0 ||
            dup2(output_pipe[1], 2) < 0)
          break;

        // Now can use stderr for errors.
        error_pipe = 2;
        close(output_pipe[1]);
      }
      // In the console case, output_pipe is still inherited by the child and
      // closed when the subprocess finishes, which then notifies ninja.

      execl("/bin/sh", "/bin/sh", "-c", command.c_str(),
            reinterpret_cast<char *>(NULL));
    } while (false);

    // If we get here, something went wrong; the execl should have
    // replaced us.
    char* err = strerror(errno);
    if (write(error_pipe, err, strlen(err)) < 0) {
      // If the write fails, there's nothing we can do.
      // But this block seems necessary to silence the warning.
    }
    _exit(1);
  }

  close(output_pipe[1]);
  return true;
}

ExitStatus AsyncSubprocess::Finish() {
  assert(pid_ != -1);
  int status;
  if (waitpid(pid_, &status, 0) < 0)
    LOG(ERROR) << "waitpid " << pid_ << ": " << strerror(errno);
  pid_ = -1;

  if (WIFEXITED(status)) {
    int exit = WEXITSTATUS(status);
    if (exit == 0)
      return ExitSuccess;
  } else if (WIFSIGNALED(status)) {
    if (WTERMSIG(status) == SIGINT)
      return ExitInterrupted;
  }
  return ExitFailure;
}

bool AsyncSubprocess::Done() const {
  return fd_ == -1;
}

const std::string& AsyncSubprocess::GetOutput() const {
  return buf_;
}

void AsyncSubprocess::OnFileCanReadWithoutBlocking(int fd) {
  if (fd != fd_) {
    NOTREACHED();
    return;
  }

  char buf[4 << 10];
  ssize_t len = read(fd_, buf, sizeof(buf));
  if (len > 0) {
    buf_.append(buf, len);
  } else {
    if (len < 0) {
      if (errno == EAGAIN)
        return;

      LOG(ERROR) << fd_ << " read: " << strerror(errno);
    }
    exit_callback_.Run(this);
    is_watching_ = false;
    fd_watcher_.StopWatchingFileDescriptor();
    close(fd_);
    fd_ = -1;
  }
}

void AsyncSubprocess::EnsureWatching() {
  if (!is_watching_ && fd_ != -1) {
    is_watching_ = base::MessageLoopForIO::current()->WatchFileDescriptor(
        fd_, true, base::MessageLoopForIO::WATCH_READ,
        &fd_watcher_, this);
  }
}

}  // namespace common
