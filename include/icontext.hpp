#pragma once

#include <chrono>

class IContext
{
protected:
  IContext() noexcept = default;

public:
  virtual ~IContext() noexcept = default;

  virtual std::chrono::time_point<std::chrono::steady_clock> get_start(void) const noexcept = 0;

  virtual std::chrono::time_point<std::chrono::steady_clock> get_stop(void) const noexcept = 0;
};
