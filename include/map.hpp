#pragma once

#include "xxhash.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

template <typename K, typename V, std::size_t N>
class Map final
{
  struct Hash final
  {
    static constexpr std::uint64_t kSeed = 0x9E3779B185EBCA87ULL;

    static inline std::uint64_t bytes(const void* data, std::size_t len) noexcept
    {
      return static_cast<std::uint64_t>(::XXH3_64bits_withSeed(data, len, kSeed));
    }

    static inline std::uint64_t of(std::string_view s) noexcept
    {
      return bytes(s.data(), s.size());
    }

    static inline std::uint64_t of(const std::string& s) noexcept
    {
      return bytes(s.data(), s.size());
    }

    template <class T>
    static inline std::uint64_t of_trivial(const T& v) noexcept
    {
      static_assert(std::is_trivially_copyable_v<T>,
                    "of_trivial requires trivially-copyable type");
      return bytes(std::addressof(v), sizeof(T));
    }

    template <class T>
    static inline std::uint64_t of(const T& key) noexcept
    {
      if constexpr (std::is_same_v<T, std::string_view>)
      {
        return of(static_cast<std::string_view>(key));
      }
      else if constexpr (std::is_same_v<T, std::string>)
      {
        return of(static_cast<const std::string&>(key));
      }
      else if constexpr (std::is_trivially_copyable_v<T>)
      {
        return of_trivial(key);
      }
      else
      {
        return static_cast<std::uint64_t>(std::hash<T>{}(key));
      }
    }
  };

  static inline std::size_t index_for_key(const K& key) noexcept
  {
    const std::uint64_t h = Hash::template of<K>(key);
    return static_cast<std::size_t>(h % static_cast<std::uint64_t>(N));
  }

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
