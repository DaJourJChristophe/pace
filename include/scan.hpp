/*
 * Responsibility - Orchestrator for scanning the stack.
 */
#pragma once

#include "frame.hpp"
#include "icontext.hpp"
#include "queue.hpp"
#include "snapshot.hpp"

#include <future>
#include <iostream>
#include <thread>

#if defined(_WIN32) || defined(__CYGWIN__)
  #include <windows.h>
#endif

class Scanner final
{
  HANDLE               m_th;
  std::future<void>    m_done;
  std::thread          m_worker;
  std::promise<HANDLE> m_th_promise;
  std::future<HANDLE>  m_th_future = m_th_promise.get_future();
  Queue<Frame, 64UL>   m_frame_buffer;
  IContext*            m_context{nullptr};

public:
  template <class T>
  Scanner(T&& target) noexcept;

  ~Scanner() noexcept;

  bool scan(const std::size_t skip       =  0UL,
            const std::size_t max_frames = 64UL) noexcept;

  Queue<Frame, 64UL>* get_frame_buffer(void) noexcept;

  void set_context(IContext* context) noexcept;
};

template <class T>
Scanner::Scanner(T&& target) noexcept
{
  std::packaged_task<void()> task([this, t = std::forward<T>(target)]() mutable
  {
  // ----- setup -----

  #if defined(_WIN32) || defined(__CYGWIN__)
    HANDLE dup = nullptr;

    BOOL ok = ::DuplicateHandle(
        ::GetCurrentProcess(),
        ::GetCurrentThread(),
        ::GetCurrentProcess(),
        &dup,
        THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION,
        FALSE,
        0);

    if (!ok)
    {
      dup = nullptr;
    }

    m_th_promise.set_value(dup);
  #else
    m_th_promise.set_value(nullptr);
  #endif

    t();
  });

  m_done   = task.get_future();
  m_worker = std::thread(std::move(task));

  m_th = m_th_future.get();
  if (!m_th)
  {
    std::cerr << "[main] failed to acquire worker thread handle\n";
    m_worker.join();
    exit(EXIT_FAILURE);
  }
}
