#include "clock.hpp"
#include "common.hpp"
#include "event.hpp"
#include "icontext.hpp"
#include "profiler.hpp"
#include "snapshot.hpp"
#include "stack.hpp"
#include "trace.hpp"
#include "trie.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <future>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

Profiler::Profiler() noexcept : m_num_captured_samples(0UL), m_previous_snapshot(), m_queue() {}

void Profiler::finalize(void) noexcept
{
  Clock& clock = Clock::get_instance();
  const std::chrono::duration<float> elapsed_seconds = (clock.get_stop() - clock.get_start());
  m_profile(elapsed_seconds.count(), {});
}

void Profiler::profile(void) noexcept
{
  auto frame_buffer = *m_frame_buffer;
  bool empty;

  for (;;)
  {
    if (frame_buffer.empty(empty))
    {
      common::fatal_trap();
    }

    if (empty)
    {
      break;
    }

    Frame frame;

    if (frame_buffer.pop(frame))
    {
      common::fatal_trap();
    }

    m_profile(frame.timestamp, frame.snapshot);
  }
}

void Profiler::profile_ERB(void) noexcept
{
  auto frame_buffer = *m_frame_buffer;
  std::size_t size;

  if (frame_buffer.size(size))
  {
    common::fatal_trap();
  }

  if (size < 32UL)
  {
    return;
  }

  bool empty;

  for (;;)
  {
    Frame frame;

    if (frame_buffer.pop(frame))
    {
      common::fatal_trap();
    }

    m_profile(frame.timestamp, frame.snapshot);

    if (frame_buffer.empty(empty))
    {
      common::fatal_trap();
    }

    if (empty)
    {
      break;
    }
  }
}

void Profiler::m_profile(const float timestamp, Snapshot snapshot) noexcept
{
  if (snapshot.empty())
  {
    if (m_previous_snapshot.empty() == false)
    {
      for (auto it = m_previous_snapshot.rbegin(); it != m_previous_snapshot.rend(); it++)
      {
        if (m_queue.emplace(EventType::END, timestamp, *it))
        {
          common::fatal_trap();
        }
      }

      m_previous_snapshot.clear();
    }

    return;
  }

  ++m_num_captured_samples;

  if (m_previous_snapshot.empty())
  {
    for (const auto& start : snapshot)
    {
      if (m_queue.emplace(EventType::START, timestamp, start))
      {
        common::fatal_trap();
      }
    }

    m_previous_snapshot = std::move(snapshot);
    return;
  }

  const auto [starts, stops] = m_find_mismatches(snapshot);

  for (const auto& start : starts)
  {
    if (m_queue.emplace(EventType::START, timestamp, start))
    {
      common::fatal_trap();
    }
  }

  for (const auto& stop : stops)
  {
    if (m_queue.emplace(EventType::END,   timestamp, stop))
    {
      common::fatal_trap();
    }
  }

  m_previous_snapshot = std::move(snapshot);
}

void Profiler::dump(void) noexcept
{
  Clock& clock = Clock::get_instance();
  const std::chrono::duration<double> elapsed_seconds = (clock.get_stop() - clock.get_start());
  const double samples_pre_second = (elapsed_seconds.count() / static_cast<double>(m_num_captured_samples));

  std::cout << std::fixed << std::setprecision(2);
  std::cout << "Captured: " << m_num_captured_samples << " samples in " << elapsed_seconds << " seconds" << std::endl;
  std::cout << "Sample rate: " << samples_pre_second << " samples/sec" << std::endl;
  std::cout << "Sampling efficiency: 0.0%" << std::endl << std::endl;
  std::cout << "Profile Stats: " << std::endl;
  std::cout << "----------------------------------------------------------------------" << std::endl;

  Stack<Event, 128> stack;

  for (;;)
  {
    Event old_event;
    Event new_event;

    bool empty;

    if (m_queue.empty(empty))
    {
      common::fatal_trap();
    }

    if (empty)
    {
      break;
    }

    if (m_queue.pop(new_event))
    {
      common::fatal_trap();
    }

    switch (new_event.type)
    {
      case EventType::START:
        assert((stack.push(new_event) == Stack<Event, 32>::kOk));
        break;

      case EventType::END:
        assert((stack.peek(old_event) == Stack<Event, 32>::kOk));
        assert(new_event.name == old_event.name);
        std::cout << new_event.name << " " << (new_event.timestamp - old_event.timestamp) << std::endl;
        assert((stack.pop(old_event) == Stack<Event, 32>::kOk));
        break;

      default:
        common::fatal_trap();
    }
  }
}

Profiler::StartsNStopsTuple Profiler::m_find_mismatches(const Snapshot& snapshot) noexcept
{
  auto p = m_previous_snapshot.begin();
  auto c =            snapshot.begin();

  std::vector<std::string> starts;
  std::vector<std::string> stops;

  while (p != m_previous_snapshot.end() && c != snapshot.end())
  {
    auto [mp, mc] = std::mismatch(p, m_previous_snapshot.end(),
                                  c,            snapshot.end());

    if (mp == m_previous_snapshot.end()  ||
        mc ==            snapshot.end())
    {
      break;
    }

     stops.push_back(*mp);
    starts.push_back(*mc);

    p = std::next(mp);
    c = std::next(mc);
  }

  for (; p != m_previous_snapshot.end(); p++)
  {
    stops.push_back(*p);
  }

  for (; c != snapshot.end(); c++)
  {
    starts.push_back(*c);
  }

  return std::make_tuple(std::move(starts), std::move(stops));
}

void Profiler::set_context(IContext* context) noexcept
{
  m_context = context;
}

void Profiler::set_frame_buffer(Queue<Frame, 64UL>* frame_buffer) noexcept
{
  m_frame_buffer = frame_buffer;
}
