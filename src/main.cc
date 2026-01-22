#include "profiler.hpp"

#include <atomic>
#include <chrono>
#include <future>
#include <thread>

#if defined(_WIN32) || defined(__CYGWIN__)
  #include <windows.h>
#endif

static void leaf()
{
  using clock = std::chrono::steady_clock;
  const auto until = clock::now() + std::chrono::seconds(2);

  volatile std::uint64_t x = 0;
  while (clock::now() < until)
  {
    x += 1;                // keep it in this function
    if ((x & 0xFFFF) == 0) // be nice-ish to the CPU
      std::this_thread::yield();
  }
}

static void mid()  { leaf(); }
static void top()  { mid(); }

#if defined(_WIN32) || defined(__CYGWIN__)
static HANDLE duplicate_current_thread_handle_for_sampling()
{
  HANDLE dup = nullptr;

  // Duplicate the current thread pseudo-handle into a real handle usable by other threads.
  // Give it the minimal rights required by your sampler.
  BOOL ok = ::DuplicateHandle(
      ::GetCurrentProcess(),
      ::GetCurrentThread(),   // pseudo handle
      ::GetCurrentProcess(),
      &dup,
      THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION,
      FALSE,
      0);

  if (!ok) return nullptr;
  return dup; // caller must CloseHandle()
}
#endif

int main()
{
  Profiler profiler;

  std::promise<HANDLE> th_promise;
  std::future<HANDLE>  th_future = th_promise.get_future();

  std::packaged_task<void()> task([&]() {
#if defined(_WIN32) || defined(__CYGWIN__)
    HANDLE th = duplicate_current_thread_handle_for_sampling();
    th_promise.set_value(th);
#else
    th_promise.set_value(nullptr);
#endif
    top();
  });

  std::future<void> done = task.get_future();
  std::thread worker(std::move(task));

  // Wait until the worker publishes its real Win32 thread handle.
  HANDLE th = th_future.get();
  if (!th)
  {
    std::cerr << "[main] failed to acquire worker thread handle\n";
    worker.join();
    return 1;
  }

  // Now sampling is truly of the worker thread.
  profiler.scan(done, th, /*skip=*/0, /*max_frames=*/64);

  worker.join();

#if defined(_WIN32) || defined(__CYGWIN__)
  ::CloseHandle(th); // we duplicated it -> we must close it
#endif

  profiler.dump();
}
