#pragma once

#include "profiler.hpp"

#include <cstdlib>
#include <future>
#include <iostream>
#include <memory>
#include <thread>

#if defined(_WIN32) || defined(__CYGWIN__)
  #include <windows.h>
#endif

namespace pace
{
  class Context final
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
    StateType               m_state_type{StateType::SCAN};
    std::shared_ptr<IState> m_state{nullptr};
    std::promise<HANDLE>    th_promise;
    std::future<HANDLE>     th_future = th_promise.get_future();
    HANDLE                  th;
    std::future<void>       done;
    std::thread             worker;

    bool m_next(void) noexcept;

    inline bool m_next_state(void) noexcept;

    inline void m_change_state(const StateType type) noexcept;

    inline void m_set_state_type(const StateType type) noexcept;

  public:
    template <class T>
    inline Context(T&& target) noexcept : m_state(StateScan::create(this))
    {
      std::packaged_task<void()> task([this, t = std::forward<T>(target)]() mutable
      {
      // ----- setup -----

      #if defined(_WIN32) || defined(__CYGWIN__)
        HANDLE dup = nullptr;

        BOOL ok = ::DuplicateHandle(
            ::GetCurrentProcess(),
            ::GetCurrentThread(),
            ::GetCurrentProcess(),
            &dup,
            THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION,
            FALSE,
            0);

        if (!ok)
        {
          dup = nullptr;
        }

        th_promise.set_value(dup);
      #else
        th_promise.set_value(nullptr);
      #endif

        t();
      });

      done   = task.get_future();
      worker = std::thread(std::move(task));

      th = th_future.get();
      if (!th)
      {
        std::cerr << "[main] failed to acquire worker thread handle\n";
        worker.join();
        exit(EXIT_FAILURE);
      }

      m_profiler.start();

      // ----- event-loop -----

      for (;;)
      {
        if (m_next())
        {
          break;
        }
      }

      m_profiler.stop();
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
