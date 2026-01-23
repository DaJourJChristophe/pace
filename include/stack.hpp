#pragma once

#include <array>
#include <cstddef>

template <typename T, std::size_t N>
class Stack
{
protected:
  std::size_t      m_size;
  std::array<T, N> m_data;

public:
  static constexpr int kFull  = (-1);
  static constexpr int kEmpty = (-2);
  static constexpr int kOk    =   0 ;

  explicit Stack() noexcept;

  [[nodiscard]] int push(const T& element) noexcept;

  template <typename... Args>
  [[nodiscard]] int emplace(Args&&... args) noexcept;

  [[nodiscard]] int peek(T& element) const noexcept;

  [[nodiscard]] int pop(T& element) noexcept;

  [[nodiscard]] bool empty(void) const noexcept;

  [[nodiscard]] std::size_t size(void) const noexcept;
};

template <typename T, std::size_t N>
Stack<T, N>::Stack() noexcept : m_size(0UL), m_data() {}

template <typename T, std::size_t N>
int Stack<T, N>::push(const T& element) noexcept
{
  if (m_size >= N)
  {
    return kFull;
  }

  m_data[m_size] = element;

  ++m_size;

  return kOk;
}

template <typename T, std::size_t N>
template <typename... Args>
int Stack<T, N>::emplace(Args&&... args) noexcept
{
  if (m_size >= N)
  {
    return kFull;
  }

  m_data[m_size] = T(std::forward<Args>(args)...);
  ++m_size;

  return kOk;
}

template <typename T, std::size_t N>
int Stack<T, N>::peek(T& element) const noexcept
{
  if (m_size == 0UL)
  {
    return kEmpty;
  }

  element = m_data[m_size - 1UL];

  return kOk;
}

template <typename T, std::size_t N>
int Stack<T, N>::pop(T& element) noexcept
{
  if (m_size == 0UL)
  {
    return kEmpty;
  }

  --m_size;

  element = m_data[m_size];

  return kOk;
}

template <typename T, std::size_t N>
bool Stack<T, N>::empty(void) const noexcept
{
  return (0UL == m_size);
}

template <typename T, std::size_t N>
std::size_t Stack<T, N>::size(void) const noexcept
{
  return m_size;
}
