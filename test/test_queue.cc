#include "queue.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

namespace
{
  template <typename T, std::size_t N>
  class MockQueue : public Queue<T, N>
  {
  public:
    MockQueue() noexcept : Queue<T, N>() {}

    const std::array<T, N>& get_data(void) const noexcept
    {
      return this->m_data;
    }

    std::uint64_t get_head(void) const noexcept
    {
      return this->m_head;
    }

    std::size_t get_size(void) const noexcept
    {
      return this->m_size;
    }

    std::uint64_t get_tail(void) const noexcept
    {
      return this->m_tail;
    }
  };
} // namespace

void test_queue_init(void)
{
  MockQueue<std::uint32_t, 4UL> queue;

  const auto&         data = queue.get_data();
  const std::size_t   size = queue.get_size();
  const std::uint64_t head = queue.get_head();
  const std::uint64_t tail = queue.get_tail();

  assert(size == 0UL);
  assert(head == 0UL);
  assert(tail == 0UL);

  for (std::uint64_t i = 0UL; i < 4UL; i++)
  {
    assert(data[i] == 0U);
  }
}

void test_queue_push(void)
{
  MockQueue<std::uint32_t, 4UL> queue;

  assert(queue.push(1) == 0);
  assert(queue.push(2) == 0);
  assert(queue.push(3) == 0);
  assert(queue.push(4) == 0);

  assert(queue.push(5) == (-1));
}

void test_queue_empty(void)
{
  MockQueue<std::uint32_t, 4UL> queue;

  bool empty = false;

  assert(queue.empty(empty) == 0 && empty == true);

  assert(queue.push(1) == 0);

  assert(queue.empty(empty) == 0 && empty == false);
}

void test_queue_size(void)
{
  MockQueue<std::uint32_t, 4UL> queue;

  std::size_t size = 0UL;

  assert(queue.size(size) == 0 && size == 0UL);

  assert(queue.push(1) == 0);

  assert(queue.size(size) == 0 && size == 1UL);
}

void test_queue_peek(void)
{
  MockQueue<std::uint32_t, 4UL> queue;

  assert(queue.push(1) == 0);
  assert(queue.push(2) == 0);
  assert(queue.push(3) == 0);
  assert(queue.push(4) == 0);

  assert(queue.push(5) == (-1));

  std::uint32_t element = 0U;

  assert(queue.peek(element) == 0 && element == 1U);
}

void test_queue_pop(void)
{
  MockQueue<std::uint32_t, 4UL> queue;

  assert(queue.push(1) == 0);
  assert(queue.push(2) == 0);
  assert(queue.push(3) == 0);
  assert(queue.push(4) == 0);

  assert(queue.push(5) == (-1));

  std::uint32_t element = 0U;

  assert(queue.pop(element) == 0 && element == 1U);
  assert(queue.pop(element) == 0 && element == 2U);
  assert(queue.pop(element) == 0 && element == 3U);
  assert(queue.pop(element) == 0 && element == 4U);

  assert(queue.pop(element) == (-2));
}

int main(void)
{
  test_queue_init();
  test_queue_push();
  test_queue_empty();
  test_queue_size();
  test_queue_peek();
  test_queue_pop();

  return EXIT_SUCCESS;
}
