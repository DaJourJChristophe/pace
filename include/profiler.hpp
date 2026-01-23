#pragma once

#include "event.hpp"
#include "queue.hpp"
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

class Profiler final
{
  struct Frame
  {
    float    timestamp;
    Snapshot snapshot;

    explicit Frame() noexcept = default;

    explicit Frame(const float timestamp_, Snapshot snapshot_) noexcept;
  };

  using StartsNStopsTuple = std::tuple<std::vector<std::string>, std::vector<std::string>>;

  std::uint64_t                                      m_num_captured_samples;
  std::chrono::time_point<std::chrono::steady_clock> m_start;
  std::chrono::time_point<std::chrono::steady_clock> m_stop;
  Snapshot                                           m_previous_snapshot;
  Queue<Event, 64UL>                                 m_queue;

  void m_profile(const float timestamp, Snapshot snapshot) noexcept;

  StartsNStopsTuple m_find_mismatches(const Snapshot& snapshot) noexcept;

  StartsNStopsTuple m_find_mismatches2(const Snapshot& snapshot) noexcept;

public:
  explicit Profiler() noexcept;

  ~Profiler() noexcept = default;

  void scan(std::future<void>& done, HANDLE th, const std::size_t skip = 0, const std::size_t max_frames = 64) noexcept;

  void dump(void) noexcept;
};
