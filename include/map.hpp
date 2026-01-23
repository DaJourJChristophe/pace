#pragma once

#include "xxhash.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

template <typename K, typename V, std::size_t N>
class Map
{
  struct Hash final
  {
    static constexpr std::uint64_t kSeed = 0x9E3779B185EBCA87ULL;

    static inline std::uint64_t bytes(const void* data, std::size_t len) noexcept;

    static inline std::uint64_t of(std::string_view s) noexcept;

    static inline std::uint64_t of(const std::string& s) noexcept;

    template <class T>
    static inline std::uint64_t of_trivial(const T& v) noexcept;

    template <class T>
    static inline std::uint64_t of(const T& key) noexcept;
  };

  static inline std::size_t m_index_for_key(const K& key) noexcept;

  using PSL = std::uint64_t;

protected:
  enum class BucketState : std::uint8_t { EMPTY, OCCUPIED };

  using BucketBase = std::uint64_t;

  struct Bucket final
  {
    BucketState state;

    BucketBase  base;
    PSL         psl;
    K           key;
    V           val;

    Bucket() noexcept;

    Bucket(const BucketState state_,
           const BucketBase  base_,
           const PSL         psl_,
           const K&          key_,
           const V&          val_) noexcept;
  };

  std::array<Bucket, N> m_slots;

public:
  Map() noexcept = default;

  int get(V& val, const K& key) noexcept;

  int set(const K& key, const V& val) noexcept;

  int del(const K& key) noexcept;
};

template <typename K, typename V, std::size_t N>
inline std::uint64_t Map<K, V, N>::Hash::bytes(const void* data, std::size_t len) noexcept
{
  return static_cast<std::uint64_t>(::XXH3_64bits_withSeed(data, len, kSeed));
}

template <typename K, typename V, std::size_t N>
inline std::uint64_t Map<K, V, N>::Hash::of(std::string_view s) noexcept
{
  return bytes(s.data(), s.size());
}

template <typename K, typename V, std::size_t N>
inline std::uint64_t Map<K, V, N>::Hash::of(const std::string& s) noexcept
{
  return bytes(s.data(), s.size());
}

template <typename K, typename V, std::size_t N>
template <class T>
inline std::uint64_t Map<K, V, N>::Hash::of_trivial(const T& v) noexcept
{
  static_assert(std::is_trivially_copyable_v<T>,
                "of_trivial requires trivially-copyable type");
  return bytes(std::addressof(v), sizeof(T));
}

template <typename K, typename V, std::size_t N>
template <class T>
inline std::uint64_t Map<K, V, N>::Hash::of(const T& key) noexcept
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

  return static_cast<std::uint64_t>(std::hash<T>{}(key));
}

template <typename K, typename V, std::size_t N>
inline std::size_t Map<K, V, N>::m_index_for_key(const K& key) noexcept
{
  const std::uint64_t h = Hash::template of<K>(key);
  return static_cast<std::size_t>(h % static_cast<std::uint64_t>(N));
}

template <typename K, typename V, std::size_t N>
Map<K, V, N>::Bucket::Bucket() noexcept
  : state(BucketState::EMPTY),
    base(0UL),
    psl(0UL),
    key(),
    val() {}

template <typename K, typename V, std::size_t N>
Map<K, V, N>::Bucket::Bucket(const BucketState state_,
                             const BucketBase  base_,
                             const PSL         psl_,
                             const K&          key_,
                             const V&          val_) noexcept
  : state(state_),
    base(base_),
    psl(psl_),
    key(key_),
    val(val_) {}

template <typename K, typename V, std::size_t N>
int Map<K, V, N>::get(V& val, const K& key) noexcept
{
  const std::uint64_t base = m_index_for_key(key);

  for (std::uint64_t displacement = 0UL; displacement < N; displacement++)
  {
    Bucket& slot = m_slots[(displacement + base) % N];

    if (slot.state == BucketState::EMPTY)
    {
      return (-1);
    }

    if (slot.state == BucketState::OCCUPIED && key == slot.key)
    {
      val = slot.val;
      return 0;
    }
  }

  return (-1);
}

template <typename K, typename V, std::size_t N>
int Map<K, V, N>::set(const K& key, const V& val) noexcept
{
  bool has_empty = false;

  for (std::uint64_t i = 0UL; i < N; i++)
  {
    Bucket& slot = m_slots[i];

    if (slot.state == BucketState::EMPTY)
    {
      has_empty = true;
      continue;
    }

    if (slot.state == BucketState::OCCUPIED && slot.key == key)
    {
      slot.val = val;
      return 0;
    }
  }

  if (!has_empty)
  {
    return (-1);
  }

  const std::uint64_t base = m_index_for_key(key);

  K           k = key;
  V           v = val;
  BucketBase  b = base;
  PSL         p = 0UL;

  for (std::uint64_t i = 0UL; i < N; i++)
  {
    Bucket& slot = m_slots[(static_cast<std::uint64_t>(b) + static_cast<std::uint64_t>(p)) % N];

    if (slot.state == BucketState::EMPTY)
    {
      slot.state = BucketState::OCCUPIED;
      slot.base  = b;
      slot.psl   = p;
      slot.key   = std::move(k);
      slot.val   = std::move(v);
      return 0;
    }

    if (slot.psl < p)
    {
      std::swap(slot.key,  k);
      std::swap(slot.val,  v);
      std::swap(slot.base, b);
      std::swap(slot.psl,  p);
      slot.state = BucketState::OCCUPIED;
    }

    ++p;
  }

  return (-1);
}

template <typename K, typename V, std::size_t N>
int Map<K, V, N>::del(const K& key) noexcept
{
  const std::uint64_t base = m_index_for_key(key);

  for (std::uint64_t displacement = 0UL; displacement < N; displacement++)
  {
    const std::uint64_t i = (displacement + base) % N;
    Bucket& slot = m_slots[i];

    if (slot.state == BucketState::EMPTY)
    {
      return (-1);
    }

    if (slot.state == BucketState::OCCUPIED && key == slot.key)
    {
      std::uint64_t hole = i;
      std::uint64_t j    = (hole + 1UL) % N;

      for (;;)
      {
        Bucket& next = m_slots[j];

        if (next.state == BucketState::EMPTY || next.psl == 0UL)
        {
          m_slots[hole] = Bucket{};
          return 0;
        }

        m_slots[hole] = std::move(next);
        m_slots[hole].psl = m_slots[hole].psl - 1UL;

        hole = j;
        j    = (j + 1UL) % N;
      }
    }
  }

  return (-1);
}
