/*
 * Responsibility - Tracking CPU time of the target process.
 */
#pragma once

#include <chrono>

class Clock
{
  std::chrono::time_point<std::chrono::steady_clock> m_start;
  std::chrono::time_point<std::chrono::steady_clock> m_stop;

  Clock() noexcept = default;

public:
  static Clock& get_instance(void) noexcept
  {
    static Clock instance;
    return instance;
  }

  Clock(const Clock&)          = delete;
  void operator=(const Clock&) = delete;
  Clock(Clock&&)               = delete;
  void operator=(Clock&&)      = delete;

  void start(void) noexcept;
  void  stop(void) noexcept;

  std::chrono::time_point<std::chrono::steady_clock> get_start(void) const noexcept;
  std::chrono::time_point<std::chrono::steady_clock> get_stop(void)  const noexcept;
};
