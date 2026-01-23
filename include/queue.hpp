#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

template <typename T, std::size_t N>
class Queue
{
protected:
  std::size_t      m_size;

  std::uint64_t    m_head;
  std::uint64_t    m_tail;

  std::array<T, N> m_data;

public:
  Queue() noexcept : m_size(0UL),
                     m_head(0UL),
                     m_tail(0UL),
                     m_data()
   {
   }
};
