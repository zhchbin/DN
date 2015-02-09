// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  THREAD_NINJA_THREAD_H_
#define  THREAD_NINJA_THREAD_H_

#include "base/callback.h"
#include "base/location.h"
#include "base/time/time.h"

namespace base {
class MessageLoopProxy;
}

class NinjaThreadDelegate;

class NinjaThread {
 public:
  // An enumeration of the well-known threads.
  enum ID {
    MAIN,

    // This is the thread that processes non-blocking IO for rpc.
    RPC,

    // This is the thread that processing file sending/receiving.
    FILE,

    // This identifier does not represent a thread.  Instead it counts the
    // number of well-known threads.  Insert new well-known threads before this
    // identifier.
    ID_COUNT
  };

  // These are the same methods in message_loop.h, but are guaranteed to either
  // get posted to the MessageLoop if it's still alive, or be deleted otherwise.
  // They return true iff the thread existed and the task was posted.  Note that
  // even if the task is posted, there's no guarantee that it will run, since
  // the target thread may already have a Quit message in its queue.
  static bool PostTask(ID identifier,
                       const tracked_objects::Location& from_here,
                       const base::Closure& task);
  static bool PostDelayedTask(ID identifier,
                              const tracked_objects::Location& from_here,
                              const base::Closure& task,
                              base::TimeDelta delay);
  static bool PostNonNestableTask(ID identifier,
                                  const tracked_objects::Location& from_here,
                                  const base::Closure& task);
  static bool PostNonNestableDelayedTask(
      ID identifier,
      const tracked_objects::Location& from_here,
      const base::Closure& task,
      base::TimeDelta delay);

  static bool PostTaskAndReply(
      ID identifier,
      const tracked_objects::Location& from_here,
      const base::Closure& task,
      const base::Closure& reply);

  // Simplified wrappers for posting to the blocking thread pool. Use this
  // for doing things like blocking I/O.
  //
  // The first variant will run the task in the pool with no sequencing
  // semantics, so may get run in parallel with other posted tasks. The second
  // variant will all post a task with no sequencing semantics, and will post a
  // reply task to the origin TaskRunner upon completion.  The third variant
  // provides sequencing between tasks with the same sequence token name.
  //
  // These tasks are guaranteed to run before shutdown.
  //
  // If you need to provide different shutdown semantics (like you have
  // something slow and noncritical that doesn't need to block shutdown),
  // or you want to manually provide a sequence token (which saves a map
  // lookup and is guaranteed unique without you having to come up with a
  // unique string), you can access the sequenced worker pool directly via
  // GetBlockingPool().
  //
  // If you need to PostTaskAndReplyWithResult, use
  // base::PostTaskAndReplyWithResult() with GetBlockingPool() as the task
  // runner.
  static bool PostBlockingPoolTask(const tracked_objects::Location& from_here,
                                   const base::Closure& task);
  static bool PostBlockingPoolTaskAndReply(
      const tracked_objects::Location& from_here,
      const base::Closure& task,
      const base::Closure& reply);
  static void ShutdownThreadPool();

  // Sets the delegate for the specified NinjaThread.
  //
  // Only one delegate may be registered at a time.  Delegates may be
  // unregistered by providing a NULL pointer.
  //
  // If the caller unregisters a delegate before CleanUp has been
  // called, it must perform its own locking to ensure the delegate is
  // not deleted while unregistering.
  static void SetDelegate(ID identifier, NinjaThreadDelegate* delegate);

  // If the current message loop is one of the known threads, returns true and
  // sets identifier to its ID.  Otherwise returns false.
  static bool GetCurrentThreadIdentifier(ID* identifier) WARN_UNUSED_RESULT;

  // Callers can hold on to a refcounted MessageLoopProxy beyond the lifetime
  // of the thread.
  static scoped_refptr<base::MessageLoopProxy> GetMessageLoopProxyForThread(
      ID identifier);

  // Callable on any thread.  Returns whether you're currently on a particular
  // thread.
  static bool CurrentlyOn(ID identifier) WARN_UNUSED_RESULT;

 private:
  friend class NinjaThreadImpl;
  NinjaThread() {}
  virtual ~NinjaThread() {}

  DISALLOW_COPY_AND_ASSIGN(NinjaThread);
};

#endif  // THREAD_NINJA_THREAD_H_
