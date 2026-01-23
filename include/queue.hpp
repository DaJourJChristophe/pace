#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

template <typename T, std::size_t N>
class Queue
{
  static std::size_t constexpr kMask = (N - 1UL);

protected:
  std::size_t      m_size;

  std::uint64_t    m_head;
  std::uint64_t    m_tail;

  std::array<T, N> m_data;

public:
  static constexpr int kFull  = (-1);
  static constexpr int kEmpty = (-2);
  static constexpr int kOk    =   0 ;

  Queue() noexcept : m_size(0UL),
                     m_head(0UL),
                     m_tail(0UL),
                     m_data()
   {
   }

   [[nodiscard]] int push(const T& element) noexcept
   {
     if (m_size >= N)
     {
       return kFull;
     }

     m_data[m_tail & kMask] = element;

     ++m_tail;
     ++m_size;

     return kOk;
   }

   [[nodiscard]] int peek(T& out) const noexcept
   {
     if (m_size == 0UL)
     {
       return kEmpty;
     }

     out = m_data[m_head & kMask];

     return kOk;
   }

   [[nodiscard]] int pop(T& out) noexcept
   {
     if (m_size == 0UL)
     {
       return kEmpty;
     }

     out = m_data[m_head & kMask];

     ++m_head;
     --m_size;

     return kOk;
   }

   [[nodiscard]] int empty(bool& out) const noexcept
   {
     out = (0UL == m_size);
     return kOk;
   }

   [[nodiscard]] int size(std::size_t& out) const noexcept
   {
     out = m_size;
     return kOk;
   }
};
