/*
 * Responsibility - Orchestrator for organizing frames produced by the Scanner by producing Events.
 */
#pragma once

#include "event.hpp"
#include "frame.hpp"
#include "icontext.hpp"
#include "queue.hpp"
#include "snapshot.hpp"

#include <cstdint>
#include <tuple>

class Profiler final
{
  using StartsNStopsTuple = std::tuple<std::vector<std::string>, std::vector<std::string>>;

  std::uint64_t       m_num_captured_samples;
  Snapshot            m_previous_snapshot;
  Queue<Event, 64UL>  m_queue;
  Queue<Frame, 64UL>* m_frame_buffer{nullptr};
  IContext*           m_context{nullptr};

  void m_profile(const float timestamp, Snapshot snapshot) noexcept;

  StartsNStopsTuple m_find_mismatches(const Snapshot& snapshot) noexcept;

public:
  explicit Profiler() noexcept;

  ~Profiler() noexcept = default;

  void finalize(void) noexcept;

  void profile(void) noexcept;

  void profile_ERB(void) noexcept;

  void dump(void) noexcept;

  void set_context(IContext* context) noexcept;

  void set_frame_buffer(Queue<Frame, 64UL>* frame_buffer) noexcept;
};
