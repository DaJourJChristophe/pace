#include "clock.hpp"
#include "common.hpp"
#include "queue.hpp"
#include "scan.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <future>
#include <thread>

#if defined(_WIN32) || defined(__CYGWIN__)
  #include <windows.h>
#endif

Scanner::~Scanner() noexcept
{
  m_worker.join();

#if defined(_WIN32) || defined(__CYGWIN__)
  ::CloseHandle(m_th);
#endif
}

bool Scanner::scan(const std::size_t skip, const std::size_t max_frames) noexcept
{
  using namespace std::chrono_literals;

  if (m_done.wait_for(0s) == std::future_status::ready)
  {
    return true;
  }

  auto frames = m_trace->capture(m_th, skip, max_frames);
  Clock& clock = Clock::get_instance();
  const auto now = std::chrono::steady_clock::now();
  const std::chrono::duration<float> elapsed_seconds = (now - clock.get_start());
  Snapshot snapshot;

  for (const auto& frame : frames)
  {
    snapshot.push_back(frame.function);
  }

  std::reverse(snapshot.begin(), snapshot.end());

  if (m_frame_buffer.emplace(elapsed_seconds.count(), std::move(snapshot)))
  {
    common::fatal_trap();
  }

  return false;
}

Queue<Frame, 64UL>* Scanner::get_frame_buffer(void) noexcept
{
  return &m_frame_buffer;
}

void Scanner::set_context(IContext* context) noexcept
{
  m_context = context;
}
