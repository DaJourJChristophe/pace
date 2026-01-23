#include "common.hpp"
#include "event.hpp"
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

Profiler::Frame::Frame(const float timestamp_, Snapshot snapshot_) noexcept
  : timestamp(timestamp_), snapshot(std::move(snapshot_)) {}

Profiler::Profiler() noexcept : m_num_captured_samples(0UL), m_previous_snapshot(), m_queue() {}

void Profiler::scan(std::future<void>& done, HANDLE th, const std::size_t skip, const std::size_t max_frames) noexcept
{
  using namespace std::chrono_literals;

  Queue<Frame, 64UL> frames_queue;

  m_start = std::chrono::steady_clock::now();

  for (;;)
  {
    auto frames = stacktrace::capture(th, skip, max_frames);
    const auto now = std::chrono::steady_clock::now();
    const std::chrono::duration<float> elapsed_seconds = (now - m_start);
    Snapshot snapshot;

    if (done.wait_for(0s) == std::future_status::ready)
    {
      bool empty;

      for (;;)
      {
        if (frames_queue.empty(empty))
        {
          common::fatal_trap();
        }

        if (empty)
        {
          break;
        }

        Frame frame;

        if (frames_queue.pop(frame))
        {
          common::fatal_trap();
        }

        m_profile(frame.timestamp, frame.snapshot);
      }

      break;
    }

    for (const auto& frame : frames)
    {
      snapshot.push_back(stacktrace::stable_function_name_only(frame));

      /*
      const auto& f = frames[i];
      std::cout << "#" << i << " 0x" << std::hex << f.pc << std::dec
                << " " << f.function;

      if (!f.file.empty())
        std::cout << " (" << f.file << ":" << f.line << ")";

      if (!f.module.empty())
        std::cout << "  [" << f.module << "]";

      if (f.offset)
        std::cout << " +0x" << std::hex << f.offset << std::dec;
      */
    }

    std::reverse(snapshot.begin(), snapshot.end());

    if (frames_queue.emplace(elapsed_seconds.count(), std::move(snapshot)))
    {
      common::fatal_trap();
    }

    std::size_t size;

    if (frames_queue.size(size))
    {
      common::fatal_trap();
    }

    if (size >= 32UL)
    {
      bool empty;

      for (;;)
      {
        if (frames_queue.empty(empty))
        {
          common::fatal_trap();
        }

        if (empty)
        {
          break;
        }

        Frame frame;

        if (frames_queue.pop(frame))
        {
          common::fatal_trap();
        }

        m_profile(frame.timestamp, frame.snapshot);
      }
    }

    std::this_thread::sleep_for(25ms);
  }

  m_stop  = std::chrono::steady_clock::now();

  const std::chrono::duration<float> elapsed_seconds = (m_stop - m_start);
  m_profile(elapsed_seconds.count(), {});
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
  const std::chrono::duration<double> elapsed_seconds = (m_stop - m_start);
  const double samples_pre_second = (elapsed_seconds.count() / static_cast<double>(m_num_captured_samples));

  std::cout << std::fixed << std::setprecision(2);
  std::cout << "Captured: " << m_num_captured_samples << " samples in " << elapsed_seconds << " seconds" << std::endl;
  std::cout << "Sample rate: " << samples_pre_second << " samples/sec" << std::endl;
  std::cout << "Sampling efficiency: 0.0%" << std::endl << std::endl;
  std::cout << "Profile Stats: " << std::endl;
  std::cout << "----------------------------------------------------------------------" << std::endl;

  Stack<Event, 128> stack;

  // while (m_queue.empty() == false)

  for (;;)
  {
    Event old_event;
    Event new_event;

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

      default: break;
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

Profiler::StartsNStopsTuple Profiler::m_find_mismatches2(const Snapshot& snapshot) noexcept
{
  // Choose a delimiter that is extremely unlikely to appear in symbol names.
  // (Unit Separator 0x1F). Any consistent delimiter works.
  static constexpr char kSep = '\x1F';

  auto append_frame = [](std::string& dst, const std::string& s) {
    dst.append(s);
    dst.push_back(kSep);
  };

  // Build the serialized previous stack (frame0<sep>frame1<sep>...)
  std::string prev_key;
  prev_key.reserve(256);
  for (const auto& f : m_previous_snapshot) append_frame(prev_key, f);

  // Insert the full previous key; trie will answer prefix-existence queries.
  HATTrie<> trie;
  trie.insert(prev_key);

  // Find LCP length in *frames* (not bytes).
  std::size_t lcp = 0;
  std::string prefix;
  prefix.reserve(prev_key.size());

  for (std::size_t i = 0; i < snapshot.size(); ++i)
  {
    append_frame(prefix, snapshot[i]);

    if (!trie.has_prefix(prefix))
      break;

    lcp = i + 1;
  }

  std::vector<std::string> starts;
  std::vector<std::string> stops;

  // Anything beyond LCP in previous => stops
  for (std::size_t i = lcp; i < m_previous_snapshot.size(); ++i)
    stops.push_back(m_previous_snapshot[i]);

  // Anything beyond LCP in current => starts
  for (std::size_t i = lcp; i < snapshot.size(); ++i)
    starts.push_back(snapshot[i]);

  return std::make_tuple(std::move(starts), std::move(stops));
}
