#include "stack.hpp"

#include <cassert>
#include <cstdint>
#include <cstdlib>

namespace
{
  template <typename T, std::size_t N>
  class MockStack : public Stack<T, N>
  {
  public:
    MockStack() noexcept : Stack<T, N>() {}

    const std::array<T, N>& get_data(void) const noexcept
    {
      return this->m_data;
    }

    std::size_t get_size(void) const noexcept
    {
      return this->m_size;
    }
  };
} // namespace

void test_stack_init(void)
{
  MockStack<std::uint32_t, 4UL> stack;

  const auto&       data = stack.get_data();
  const std::size_t size = stack.get_size();

  assert(size == 0UL);

  for (std::uint64_t i = 0UL; i < 4UL; i++)
  {
    assert(data[i] == 0U);
  }
}

void test_stack_push(void)
{
  MockStack<std::uint32_t, 4UL> stack;

  assert(stack.push(1) == 0);
  assert(stack.push(2) == 0);
  assert(stack.push(3) == 0);
  assert(stack.push(4) == 0);

  assert(stack.push(5) == (-1));
}

void test_stack_empty(void)
{
  MockStack<std::uint32_t, 4UL> stack;

  assert(stack.empty() == true);

  assert(stack.push(1) == 0);

  assert(stack.empty() == false);
}

void test_stack_size(void)
{
  MockStack<std::uint32_t, 4UL> stack;

  assert(stack.size() == 0UL);

  assert(stack.push(1) == 0);

  assert(stack.size() == 1UL);
}

void test_stack_peek(void)
{
  MockStack<std::uint32_t, 4UL> stack;

  assert(stack.push(1) == 0);
  assert(stack.push(2) == 0);
  assert(stack.push(3) == 0);
  assert(stack.push(4) == 0);

  assert(stack.push(5) == (-1));

  std::uint32_t element = 0U;

  assert(stack.peek(element) == 0 && element == 4U);
}

void test_stack_pop(void)
{
  MockStack<std::uint32_t, 4UL> stack;

  assert(stack.push(1) == 0);
  assert(stack.push(2) == 0);
  assert(stack.push(3) == 0);
  assert(stack.push(4) == 0);

  assert(stack.push(5) == (-1));

  std::uint32_t element = 0U;

  assert(stack.pop(element) == 0 && element == 4U);
  assert(stack.pop(element) == 0 && element == 3U);
  assert(stack.pop(element) == 0 && element == 2U);
  assert(stack.pop(element) == 0 && element == 1U);

  assert(stack.pop(element) == (-2));
}

int main(void)
{
  test_stack_init();
  test_stack_push();
  test_stack_empty();
  test_stack_size();
  test_stack_peek();
  test_stack_pop();

  return EXIT_SUCCESS;
}
