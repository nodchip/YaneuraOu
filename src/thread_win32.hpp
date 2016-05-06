#ifndef APERY_THREAD_WIN32_HPP
#define APERY_THREAD_WIN32_HPP

/// STL thread library used by mingw and gcc when cross compiling for Windows
/// relies on libwinpthread. Currently libwinpthread implements mutexes directly
/// on top of Windows semaphores. Semaphores, being kernel objects, require kernel
/// mode transition in order to lock or unlock, which is very slow compared to
/// interlocked operations (about 30% slower on bench test). To workaround this
/// issue, we define our wrappers to the low level Win32 calls. We use critical
/// sections to support Windows XP and older versions. Unfortunately, cond_wait()
/// is racy between unlock() and WaitForSingleObject() but they have the same
/// speed performance of SRW locks.

#include <condition_variable>
#include <mutex>

#if defined(_WIN32) && !defined(_MSC_VER)

#ifndef NOMINMAX
#  define NOMINMAX // Disable macros min() and max()
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX

/// Mutex and ConditionVariable struct are wrappers of the low level locking
/// machinery and are modeled after the corresponding C++11 classes.

struct Mutex {
  Mutex() { InitializeCriticalSection(&cs); }
 ~Mutex() { DeleteCriticalSection(&cs); }
  void lock() { EnterCriticalSection(&cs); }
  void unlock() { LeaveCriticalSection(&cs); }

private:
  CRITICAL_SECTION cs;
};

typedef std::condition_variable_any ConditionVariable;

#else // Default case: use STL classes

typedef std::mutex Mutex;
typedef std::condition_variable ConditionVariable;

#endif

#endif // #ifndef THREAD_WIN32_H_INCLUDED
