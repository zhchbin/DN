// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "common/async_subprocess.h"

#include <stdio.h>

namespace common {

AsyncSubprocess::AsyncSubprocess()
    : use_console_(false),
      context_(NULL),
      is_reading_(false) {
}

AsyncSubprocess::~AsyncSubprocess() {
  if (pipe_) {
    if (!CloseHandle(pipe_))
      NOTREACHED() << "CloseHandle";
  }

  // Reap child if forgotten.
  if (child_)
    Finish();
}

bool AsyncSubprocess::Start(const std::string& command,
                            const ExitCallback& callback) {
  return Start(command, callback, false);
}

bool AsyncSubprocess::Start(const std::string& command,
                            const ExitCallback& callback,
                            bool use_console) {
  CHECK(context_ == NULL) << "Don't start subprocess twice.";
  context_ = new base::MessageLoopForIO::IOContext();
  context_->handler = this;
  memset(&context_->overlapped, 0, sizeof(context_->overlapped));

  use_console_ = use_console;
  exit_callback_ = callback;

  HANDLE child_pipe = SetupPipe();
  SECURITY_ATTRIBUTES security_attributes;
  memset(&security_attributes, 0, sizeof(SECURITY_ATTRIBUTES));
  security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
  security_attributes.bInheritHandle = TRUE;
  // Must be inheritable so subprocesses can dup to children.
  HANDLE nul = CreateFile(L"NUL", GENERIC_READ,
          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
          &security_attributes, OPEN_EXISTING, 0, NULL);
  if (nul == INVALID_HANDLE_VALUE) {
    NOTREACHED() << "couldn't open nul";
    return false;
  }

  STARTUPINFOA startup_info;
  memset(&startup_info, 0, sizeof(startup_info));
  startup_info.cb = sizeof(STARTUPINFO);
  if (!use_console_) {
    startup_info.dwFlags = STARTF_USESTDHANDLES;
    startup_info.hStdInput = nul;
    startup_info.hStdOutput = child_pipe;
    startup_info.hStdError = child_pipe;
  }
  // In the console case, child_pipe is still inherited by the child and closed
  // when the subprocess finishes, which then notifies ninja.

  PROCESS_INFORMATION process_info;
  memset(&process_info, 0, sizeof(process_info));

  // Ninja handles ctrl-c, except for subprocesses in console pools.
  DWORD process_flags = use_console_ ? 0 : CREATE_NEW_PROCESS_GROUP;

  // Do not prepend 'cmd /c' on Windows, this breaks command
  // lines greater than 8,191 chars.
  if (!CreateProcessA(NULL, strdup(command.c_str()), NULL,
                      NULL, /* inherit handles */ TRUE, process_flags,
                      NULL, NULL,
                      &startup_info, &process_info)) {
    DWORD error = GetLastError();
    if (error == ERROR_FILE_NOT_FOUND) {
      // File (program) not found error is treated as a normal build
      // action failure.
      if (child_pipe)
        CloseHandle(child_pipe);
      CloseHandle(pipe_);
      CloseHandle(nul);
      pipe_ = NULL;
      // child_ is already NULL;
      buf_ = "CreateProcess failed: The system cannot find the file "
          "specified.\n";
      return true;
    } else {
      NOTREACHED() << "CreateProcess";
      return false;
    }
  }

  // Close pipe channel only used by the child.
  if (child_pipe)
    CloseHandle(child_pipe);
  CloseHandle(nul);

  CloseHandle(process_info.hThread);
  child_ = process_info.hProcess;
  return true;
}

ExitStatus AsyncSubprocess::Finish() {
  if (!child_)
    return ExitFailure;

  WaitForSingleObject(child_, INFINITE);
  DWORD exit_code = 0;
  GetExitCodeProcess(child_, &exit_code);
  CloseHandle(child_);
  child_ = NULL;

  return exit_code == 0              ? ExitSuccess :
         exit_code == CONTROL_C_EXIT ? ExitInterrupted :
                                       ExitFailure;
}

bool AsyncSubprocess::Done() const {
  return pipe_ == NULL;
}

const std::string& AsyncSubprocess::GetOutput() const {
  return buf_;
}

void AsyncSubprocess::OnPipeReady() {
  DWORD bytes;
  if (!GetOverlappedResult(pipe_, &context_->overlapped, &bytes, TRUE)) {
    if (GetLastError() == ERROR_BROKEN_PIPE) {
      CloseHandle(pipe_);
      pipe_ = NULL;
      return;
    }
    NOTREACHED() << "GetOverlappedResult";
  }

  if (is_reading_ && bytes)
    buf_.append(overlapped_buf_, bytes);

  memset(&context_->overlapped, 0, sizeof(context_->overlapped));
  is_reading_ = true;
  if (!::ReadFile(pipe_, overlapped_buf_, sizeof(overlapped_buf_),
                  &bytes, &context_->overlapped)) {
    if (GetLastError() == ERROR_BROKEN_PIPE) {
      CloseHandle(pipe_);
      pipe_ = NULL;
      return;
    }
    if (GetLastError() != ERROR_IO_PENDING)
      NOTREACHED() << "ReadFile";
  }
  // Even if we read any bytes in the readfile call, we'll enter this
  // function again later and get them at that point.
}

// Implementation of IOHandler on Windows.
void AsyncSubprocess::OnIOCompleted(
    base::MessageLoopForIO::IOContext* context,
    DWORD bytes_transfered,
    DWORD error) {
  DCHECK_EQ(context_, context);
  DCHECK(!exit_callback_.is_null());
  OnPipeReady();
  if (Done())
    exit_callback_.Run(this);
}

HANDLE AsyncSubprocess::SetupPipe() {
  wchar_t pipe_name[100];
  swprintf(pipe_name, L"\\\\.\\pipe\\ninja_pid%lu_sp%p",
           GetCurrentProcessId(), this);

  pipe_ = ::CreateNamedPipe(pipe_name,
                            PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
                            PIPE_TYPE_BYTE,
                            PIPE_UNLIMITED_INSTANCES,
                            0, 0, INFINITE, NULL);
  DCHECK_NE(pipe_, INVALID_HANDLE_VALUE) << "CreateNamedPipe";
  if (!ConnectNamedPipe(pipe_, &context_->overlapped) &&
      GetLastError() != ERROR_IO_PENDING) {
    NOTREACHED() << "ConnectNamedPipe";
  }
  base::MessageLoopForIO::current()->RegisterIOHandler(pipe_, this);

  // Get the write end of the pipe as a handle inheritable across processes.
  HANDLE output_write_handle = CreateFile(pipe_name, GENERIC_WRITE, 0,
                                          NULL, OPEN_EXISTING, 0, NULL);
  HANDLE output_write_child;
  if (!DuplicateHandle(GetCurrentProcess(), output_write_handle,
                       GetCurrentProcess(), &output_write_child,
                       0, TRUE, DUPLICATE_SAME_ACCESS)) {
    NOTREACHED() << "DuplicateHandle";
  }
  CloseHandle(output_write_handle);

  return output_write_child;
}

}  // namespace common
