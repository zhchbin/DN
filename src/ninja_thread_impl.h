#ifndef  NINJA_THREAD_IMPL_H_
#define  NINJA_THREAD_IMPL_H_

#include "base/threading/thread.h"
#include "ninja_thread.h"

class NinjaThreadImpl : public NinjaThread, public base::Thread {
 public:
  // Construct a NinjaThreadImpl with the supplied identifier.  It is an error
  // to construct a NinjaThreadImpl that already exists.
  explicit NinjaThreadImpl(NinjaThread::ID identifier);

  // Special constructor for the main (UI) thread and unittests. If a
  // |message_loop| is provied, we use a dummy thread here since the main
  // thread already exists.
  NinjaThreadImpl(NinjaThread::ID identifier, base::MessageLoop* message_loop);
  virtual ~NinjaThreadImpl() override;

 protected:
  // base::Thread implementation.
  void Init() override;
  void Run(base::MessageLoop* message_loop) override;
  void CleanUp() override;

 private:
  friend class NinjaThread;

  // Common initialization code for the constructors.
  void Initialize();

  // The following are unique function names that makes it possible to tell
  // the thread id from the callstack alone in crash dumps.
  void MainThreadRun(base::MessageLoop* message_loop);
  void RpcIOThreadRun(base::MessageLoop* message_loop);

  static bool PostTaskHelper(
      NinjaThread::ID identifier,
      const tracked_objects::Location& from_here,
      const base::Closure& task,
      base::TimeDelta delay,
      bool nestable);

  // The identifier of this thread.  Only one thread can exist with a given
  // identifier at a given time.
  ID identifier_;
};

#endif  // NINJA_THREAD_IMPL_H_
