// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "thread/ninja_thread_impl.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/lazy_instance.h"
#include "base/synchronization/lock.h"
#include "thread/ninja_thread_delegate.h"

namespace {

// Friendly names for the well-known threads.
static const char* g_ninja_thread_names[NinjaThread::ID_COUNT] = {
  "Main", "RPC",
};

struct NinjaThreadGlobals {
  NinjaThreadGlobals() {
    memset(threads, 0, NinjaThread::ID_COUNT * sizeof(threads[0]));
    memset(thread_delegates, 0,
           NinjaThread::ID_COUNT * sizeof(thread_delegates[0]));
  }

  // This lock protects |threads|. Do not read or modify that array
  // without holding this lock. Do not block while holding this lock.
  base::Lock lock;

  // Only atomic operations are used on this array. The delegates are not owned
  // by this array, rather by whoever calls BrowserThread::SetDelegate.
  NinjaThreadDelegate* thread_delegates[NinjaThread::ID_COUNT];

  // This array is protected by |lock|. The threads are not owned by this
  // array. Typically, the threads are owned on the Main thread.
  // NinjaThreadImpl objects remove themselves from this array upon
  // destruction.
  NinjaThreadImpl* threads[NinjaThread::ID_COUNT];
};

base::LazyInstance<NinjaThreadGlobals>::Leaky
    g_globals = LAZY_INSTANCE_INITIALIZER;

}  // namespace

NinjaThreadImpl::NinjaThreadImpl(NinjaThread::ID identifier)
    : base::Thread(g_ninja_thread_names[identifier]),
      identifier_(identifier) {
  Initialize();
}

NinjaThreadImpl::NinjaThreadImpl(NinjaThread::ID identifier,
                                 base::MessageLoop* message_loop)
    : base::Thread(g_ninja_thread_names[identifier]),
      identifier_(identifier) {
  set_message_loop(message_loop);
  Initialize();
}

NinjaThreadImpl::~NinjaThreadImpl() {
  // All Thread subclasses must call Stop() in the destructor. This is
  // doubly important here as various bits of code check they are on
  // the right NinjaThread.
  Stop();

  NinjaThreadGlobals& globals = g_globals.Get();
  base::AutoLock lock(globals.lock);
  globals.threads[identifier_] = NULL;
#ifndef NDEBUG
  // Double check that the threads are ordered correctly in the enumeration.
  for (int i = identifier_ + 1; i < ID_COUNT; ++i) {
    DCHECK(!globals.threads[i]) <<
        "Threads must be listed in the reverse order that they die";
  }
#endif
}

void NinjaThreadImpl::Init() {
  NinjaThreadGlobals& globals = g_globals.Get();

  using base::subtle::AtomicWord;
  AtomicWord* storage =
      reinterpret_cast<AtomicWord*>(&globals.thread_delegates[identifier_]);
  AtomicWord stored_pointer = base::subtle::NoBarrier_Load(storage);
  NinjaThreadDelegate* delegate =
      reinterpret_cast<NinjaThreadDelegate*>(stored_pointer);
  if (delegate) {
    delegate->Init();
    message_loop()->PostTask(FROM_HERE,
                             base::Bind(&NinjaThreadDelegate::InitAsync,
                                        // Delegate is expected to exist for the
                                        // duration of the thread's lifetime
                                        base::Unretained(delegate)));
  }
}

void NinjaThreadImpl::Run(base::MessageLoop* message_loop) {
  NinjaThread::ID thread_id = ID_COUNT;
  if (!GetCurrentThreadIdentifier(&thread_id))
    return Thread::Run(message_loop);

  switch (thread_id) {
    case NinjaThread::MAIN:
      return MainThreadRun(message_loop);
    case NinjaThread::RPC:
      return RpcIOThreadRun(message_loop);
    case NinjaThread::ID_COUNT:
      CHECK(false);  // This shouldn't actually be reached!
      break;
  }
  Thread::Run(message_loop);
}

void NinjaThreadImpl::CleanUp() {
  NinjaThreadGlobals& globals = g_globals.Get();

  using base::subtle::AtomicWord;
  AtomicWord* storage =
      reinterpret_cast<AtomicWord*>(&globals.thread_delegates[identifier_]);
  AtomicWord stored_pointer = base::subtle::NoBarrier_Load(storage);
  NinjaThreadDelegate* delegate =
      reinterpret_cast<NinjaThreadDelegate*>(stored_pointer);

  if (delegate)
    delegate->CleanUp();
}

void NinjaThreadImpl::Initialize() {
  NinjaThreadGlobals& globals = g_globals.Get();

  base::AutoLock lock(globals.lock);
  DCHECK(identifier_ >= 0 && identifier_ < ID_COUNT);
  DCHECK(globals.threads[identifier_] == NULL);
  globals.threads[identifier_] = this;
}

NOINLINE void NinjaThreadImpl::MainThreadRun(base::MessageLoop* message_loop) {
  volatile int line_number = __LINE__;
  Thread::Run(message_loop);
  CHECK_GT(line_number, 0);
}

NOINLINE void NinjaThreadImpl::RpcIOThreadRun(base::MessageLoop* message_loop) {
  volatile int line_number = __LINE__;
  Thread::Run(message_loop);
  CHECK_GT(line_number, 0);
}

// static
bool NinjaThreadImpl::PostTaskHelper(
    NinjaThread::ID identifier,
    const tracked_objects::Location& from_here,
    const base::Closure& task,
    base::TimeDelta delay,
    bool nestable) {
  DCHECK(identifier >= 0 && identifier < ID_COUNT);
  NinjaThread::ID current_thread = ID_COUNT;
  bool target_thread_outlives_current =
      GetCurrentThreadIdentifier(&current_thread) &&
      current_thread >= identifier;

  NinjaThreadGlobals& globals = g_globals.Get();
  if (!target_thread_outlives_current)
    globals.lock.Acquire();

  base::MessageLoop* message_loop =
      globals.threads[identifier] ? globals.threads[identifier]->message_loop()
                                  : NULL;
  if (message_loop) {
    if (nestable) {
      message_loop->PostDelayedTask(from_here, task, delay);
    } else {
      message_loop->PostNonNestableDelayedTask(from_here, task, delay);
    }
  }

  if (!target_thread_outlives_current)
    globals.lock.Release();

  return !!message_loop;
}

// static
bool NinjaThread::PostTask(ID identifier,
                           const tracked_objects::Location& from_here,
                           const base::Closure& task) {
  return NinjaThreadImpl::PostTaskHelper(
      identifier, from_here, task, base::TimeDelta(), true);
}

// static
bool NinjaThread::PostDelayedTask(ID identifier,
                                  const tracked_objects::Location& from_here,
                                  const base::Closure& task,
                                  base::TimeDelta delay) {
  return NinjaThreadImpl::PostTaskHelper(
      identifier, from_here, task, delay, true);
}

// static
bool NinjaThread::PostNonNestableTask(
    ID identifier,
    const tracked_objects::Location& from_here,
    const base::Closure& task) {
  return NinjaThreadImpl::PostTaskHelper(
      identifier, from_here, task, base::TimeDelta(), false);
}

// static
void NinjaThread::SetDelegate(ID identifier,
                              NinjaThreadDelegate* delegate) {
  using base::subtle::AtomicWord;
  NinjaThreadGlobals& globals = g_globals.Get();
  AtomicWord* storage = reinterpret_cast<AtomicWord*>(
      &globals.thread_delegates[identifier]);
  AtomicWord old_pointer = base::subtle::NoBarrier_AtomicExchange(
      storage, reinterpret_cast<AtomicWord>(delegate));

  // This catches registration when previously registered.
  DCHECK(!delegate || !old_pointer);
}

// static
bool NinjaThread::GetCurrentThreadIdentifier(ID* identifier) {
  if (g_globals == NULL)
    return false;

  base::ThreadRestrictions::ScopedAllowSingleton allow_singleton;
  base::MessageLoop* cur_message_loop = base::MessageLoop::current();
  NinjaThreadGlobals& globals = g_globals.Get();
  for (int i = 0; i < ID_COUNT; ++i) {
    if (globals.threads[i] &&
        globals.threads[i]->message_loop() == cur_message_loop) {
      *identifier = globals.threads[i]->identifier_;
      return true;
    }
  }

  return false;
}

// static
bool NinjaThread::CurrentlyOn(ID identifier) {
  // We shouldn't use MessageLoop::current() since it uses LazyInstance which
  // may be deleted by ~AtExitManager when a WorkerPool thread calls this
  // function.
  base::ThreadRestrictions::ScopedAllowSingleton allow_singleton;
  NinjaThreadGlobals& globals = g_globals.Get();
  base::AutoLock lock(globals.lock);
  DCHECK(identifier >= 0 && identifier < ID_COUNT);
  return globals.threads[identifier] &&
         globals.threads[identifier]->message_loop() ==
             base::MessageLoop::current();
}
