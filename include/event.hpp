#pragma once

#include <cstdint>
#include <iostream>
#include <string>

enum class EventType : std::uint8_t { START, END };

struct Event final
{
  EventType   type;
  float       timestamp;
  std::string name;

  explicit Event() noexcept = default;

  explicit Event(const EventType    type_,
                 const float        timestamp_,
                 const std::string& name_) noexcept;

  void print(void) const noexcept;
};
