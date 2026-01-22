#include "event.hpp"

#include <cstdint>
#include <iostream>
#include <string>

Event::Event(const EventType    type_,
             const float        timestamp_,
             const std::string& name_) noexcept
  : type(type_),
    timestamp(timestamp_),
    name(std::move(name_)) {}

void Event::print(void) const noexcept
{
  std::cout << "{ type: ";

  switch (type)
  {
    case EventType::START:
      std::cout << "START";
      break;

    case EventType::END:
      std::cout << "END  ";
      break;

    default: break;
  }

  std::cout << ", timestamp: " << timestamp << ", name: " << name << " }" << std::endl;
}
