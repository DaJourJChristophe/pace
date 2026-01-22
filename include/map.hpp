#pragma once

#include <array>
#include <cstdint>

template <typename K, typename V, std::size_t N>
class Map final
{
  enum class BucketState : std::uint8_t
  {
    EMPTY,
    OCCUPIED,
    TOMBSTONE,
  };

  struct Bucket final
  {
    BucketState state;
    K           key;
    V           val;
  };

  std::array<Bucket, N> m_slots;

public:
  Map() noexcept = default;

  int get(void) noexcept { return (-1); }

  int set(void) noexcept { return (-1); }

  int del(void) noexcept { return (-1); }
};
