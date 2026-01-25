#pragma once

#include "queue.hpp"

#include <iostream>
#include <string>

class Diagnostic final
{
  struct Event final
  {
    std::string message;
  };

  Queue<Event, 4UL> m_buffer;

public:
  Diagnostic() noexcept = default;

  void dump(void) noexcept
  {
    for (;;)
    {
      bool empty;

      (void)m_buffer.empty(empty);

      if (empty)
      {
        break;
      }

      Event event;

      (void)m_buffer.pop(event);

      std::cout << event.message << std::endl;
    }
  }

  void push(const std::string& message) noexcept
  {
    (void)m_buffer.emplace(message);
  }
};
