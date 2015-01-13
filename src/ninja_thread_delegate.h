// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  NINJA_THREAD_DELEGATE_H_
#define  NINJA_THREAD_DELEGATE_H_

// A class with this type may be registered via NinjaThread::SetDelegate.
//
// If registered as such, it will schedule to run Init() before the
// message loop begins and the schedule InitAsync() as the first
// task on its message loop (after the NinjaThread has done its own
// initialization), and receive a CleanUp call right after the message
// loop ends (and before the NinjaThread has done its own clean-up).
class NinjaThreadDelegate {
 public:
  virtual ~NinjaThreadDelegate() {}

  // Called prior to starting the message loop
  virtual void Init() = 0;

  // Called as the first task on the thread's message loop.
  virtual void InitAsync() = 0;

  // Called just after the message loop ends.
  virtual void CleanUp() = 0;
};
 
#endif  // NINJA_THREAD_DELEGATE_H_
