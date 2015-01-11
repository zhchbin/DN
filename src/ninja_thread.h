#ifndef  NINJA_THREAD_H_
#define  NINJA_THREAD_H_

#include "base/callback.h"
#include "base/location.h"
#include "base/time/time.h"

class NinjaThreadDelegate;

class NinjaThread {
 public:
  // An enumeration of the well-known threads. 
  enum ID {
    MAIN,

    // This is the thread that processes non-blocking IO for rpc.
    RPC_IO,

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

 private:
  friend class NinjaThreadImpl;

  NinjaThread() {}
  virtual ~NinjaThread() {}
  DISALLOW_COPY_AND_ASSIGN(NinjaThread);
};

#endif  // NINJA_THREAD_H_
