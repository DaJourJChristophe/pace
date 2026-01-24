#include "context.hpp"

#include <chrono>
#include <thread>

static void leaf(void)
{
  using clock = std::chrono::steady_clock;
  const auto until = clock::now() + std::chrono::seconds(2);

  volatile std::uint64_t x = 0;

  while (clock::now() < until)
  {
    x += 1;

    if ((x & 0xFFFF) == 0)
    {
      std::this_thread::yield();
    }
  }
}

static void mid(void)
{
  leaf();
}

static void top(void)
{
  mid();
}

int main(void)
{
  pace::Context ctx(top);
  return 0;
}
