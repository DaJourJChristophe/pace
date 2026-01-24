/*
 * Responsibility - A container orchestrator for interacting with subsystem components.
 */
#pragma once

#include "clock.hpp"
#include "icontext.hpp"
#include "profiler.hpp"
#include "scan.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>

#if defined(_WIN32) || defined(__CYGWIN__)
  #include <windows.h>
#endif

namespace pace
{
  class Context final : public IContext
  {
    enum class StateType { SCAN, PROFILE, THROTTLE, EXIT };

    class IState
    {
    protected:
      IState() noexcept = default;

    public:
      virtual ~IState() noexcept = default;

      virtual bool next(void) noexcept = 0;
    };

    class AState : public IState
    {
    protected:
      Context* m_ctx{nullptr};

      AState(Context* ctx) noexcept;
    };

    class StateScan final : public AState
    {
    public:
      StateScan(Context* ctx) noexcept;

      bool next(void) noexcept override;

      static inline std::shared_ptr<IState> create(Context* ctx);
    };

    class StateProfile final : public AState
    {
    public:
      StateProfile(Context* ctx) noexcept;

      bool next(void) noexcept override;

      static inline std::shared_ptr<IState> create(Context* ctx);
    };

    class StateThrottle final : public AState
    {
    public:
      StateThrottle(Context* ctx) noexcept;

      bool next(void) noexcept override;

      static inline std::shared_ptr<IState> create(Context* ctx);
    };

    Profiler                m_profiler;
    Scanner                 m_scanner;
    StateType               m_state_type{StateType::SCAN};
    std::shared_ptr<IState> m_state{nullptr};

    bool m_next(void) noexcept;

    inline bool m_next_state(void) noexcept;

    inline void m_change_state(const StateType type) noexcept;

    inline void m_set_state_type(const StateType type) noexcept;

  public:
    template <class T>
    inline Context(T&& target) noexcept : m_profiler(),
                                          m_scanner(target),
                                          m_state(StateScan::create(this))
    {
      m_scanner.set_context(this);

      auto frame_buffer = m_scanner.get_frame_buffer();

      m_profiler.set_context(this);
      m_profiler.set_frame_buffer(frame_buffer);

      Clock& clock = Clock::get_instance();
      clock.start();

      for (;;)
      {
        if (m_next())
        {
          break;
        }
      }

      clock.stop();

      m_profiler.finalize();
    }

    ~Context() noexcept;

    void profile(void) noexcept;

    void profile_ERB(void) noexcept;

    bool scan(void) noexcept;
  };

  inline std::shared_ptr<Context::IState> Context::StateScan::create(Context* ctx)
  {
    return std::make_shared<StateScan>(ctx);
  }

  inline std::shared_ptr<Context::IState> Context::StateProfile::create(Context* ctx)
  {
    return std::make_shared<StateProfile>(ctx);
  }

  inline std::shared_ptr<Context::IState> Context::StateThrottle::create(Context* ctx)
  {
    return std::make_shared<StateThrottle>(ctx);
  }
} // namespace pace
