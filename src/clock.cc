#include "clock.hpp"

#include <chrono>

void Clock::start(void) noexcept
{
  m_start = std::chrono::steady_clock::now();
}

void Clock::stop(void) noexcept
{
  m_stop = std::chrono::steady_clock::now();
}

std::chrono::time_point<std::chrono::steady_clock> Clock::get_start(void) const noexcept
{
  return m_start;
}

std::chrono::time_point<std::chrono::steady_clock> Clock::get_stop(void)  const noexcept
{
  return m_stop;
}
