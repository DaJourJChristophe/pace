#include "common.hpp"
#include "context.hpp"

#include <chrono>
#include <cstdlib>
#include <memory>

#if defined(_WIN32) || defined(__CYGWIN__)
  #include <windows.h>
#endif

namespace pace
{
  Context::AState::AState(Context* ctx) noexcept : m_ctx(ctx) {}

  Context::StateScan::StateScan(Context* ctx) noexcept : AState(ctx) {}

  bool Context::StateScan::next(void) noexcept
  {
    return m_ctx->scan();
  }

  Context::StateProfile::StateProfile(Context* ctx) noexcept : AState(ctx) {}

  bool Context::StateProfile::next(void) noexcept
  {
    m_ctx->profile_ERB();
    return false;
  }

  Context::StateThrottle::StateThrottle(Context* ctx) noexcept : AState(ctx) {}

  bool Context::StateThrottle::next(void) noexcept
  {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(25ms);
    return false;
  }

  bool Context::m_next(void) noexcept
  {
    return m_next_state();
  }

  inline bool Context::m_next_state(void) noexcept
  {
    if (m_state == nullptr)
    {
      common::fatal_trap();
    }

    if (m_state->next())  // NOTE: This is kind of cheating..
    {
      m_set_state_type(StateType::EXIT);
    }

    switch (m_state_type)
    {
      case StateType::EXIT:
        m_profiler.profile();
        return true;

      case StateType::SCAN:
        m_set_state_type(StateType::PROFILE);
          m_change_state(StateType::PROFILE);
        break;

      case StateType::PROFILE:
        m_set_state_type(StateType::THROTTLE);
          m_change_state(StateType::THROTTLE);
        break;

      case StateType::THROTTLE:
        m_set_state_type(StateType::SCAN);
          m_change_state(StateType::SCAN);
        break;

      default:
        common::fatal_trap();
    }

    return false;
  }

  inline void Context::m_change_state(const StateType type) noexcept
  {
    switch (type)
    {
      case StateType::SCAN:
        m_state =     StateScan::create(this);
        break;

      case StateType::PROFILE:
        m_state =  StateProfile::create(this);
        break;

      case StateType::THROTTLE:
        m_state = StateThrottle::create(this);
        break;

      default:
        common::fatal_trap();
    }
  }

  inline void Context::m_set_state_type(const StateType type) noexcept
  {
    m_state_type = type;
  }

  Context::~Context() noexcept
  {
    m_profiler.dump();
  }

  void Context::profile(void) noexcept
  {
    m_profiler.profile();
  }

  void Context::profile_ERB(void) noexcept
  {
    m_profiler.profile_ERB();
  }

  bool Context::scan(void) noexcept
  {
    return m_scanner.scan(/*skip=*/0UL, /*max_frames=*/64UL);
  }
} // namespace pace
