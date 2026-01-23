#include "stack.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

template <typename F>
void measure_elapsed(const char* label, F&& callback)
{
  const auto start    = std::chrono::steady_clock::now();

  callback();

  const auto end      = std::chrono::steady_clock::now();
  const auto duration =
    std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  std::cout << label << ": " << duration.count() << " ms\n";
}

static volatile std::uint64_t g_sink = 0;

static inline char rand_char(std::mt19937_64& rng) noexcept
{
  static constexpr char alphabet[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789_";

  std::uniform_int_distribution<std::size_t> dist(0, sizeof(alphabet) - 2);
  return alphabet[dist(rng)];
}

static std::string make_random_key(std::mt19937_64& rng,
                                   std::size_t min_len,
                                   std::size_t max_len)
{
  std::uniform_int_distribution<std::size_t> len_dist(min_len, max_len);
  const std::size_t len = len_dist(rng);

  std::string s;
  s.resize(len);

  for (std::size_t i = 0; i < len; i++)
  {
    s[i] = rand_char(rng);
  }

  return s;
}

static std::string make_prefix_heavy_key(std::mt19937_64& rng,
                                         std::string_view common_prefix,
                                         std::size_t min_tail,
                                         std::size_t max_tail)
{
  std::uniform_int_distribution<std::size_t> len_dist(min_tail, max_tail);
  const std::size_t tail = len_dist(rng);

  std::string s;

  s.reserve(common_prefix.size() + tail);
  s.append(common_prefix);

  for (std::size_t i = 0; i < tail; i++)
  {
    s.push_back(rand_char(rng));
  }

  return s;
}

static std::vector<std::string> generate_keys(std::size_t count,
                                              bool prefix_heavy,
                                              std::uint64_t seed = 0xC0FFEEULL)
{
  std::mt19937_64 rng(seed);

  std::vector<std::string> keys;
  keys.reserve(count);

  if (prefix_heavy)
  {
    const std::string prefix = "common/prefix/";

    for (std::size_t i = 0; i < count; i++)
    {
      keys.push_back(make_prefix_heavy_key(rng, prefix, 4, 24));
    }
  }
  else
  {
    for (std::size_t i = 0; i < count; i++)
    {
      keys.push_back(make_random_key(rng, 8, 32));
    }
  }

  return keys;
}

static void stress_insert(Stack<std::string, 16UL>& stack, const std::vector<std::string>& keys)
{
  measure_elapsed("insert bulk", [&]
  {
    for (const auto& k : keys)
    {
      (void)stack.push(k);
    }
  });
}

int main(void)
{
  const std::size_t num_keys     = 200'000;
  // const std::size_t num_queries  = 1'000'000;
  const bool        prefix_heavy = true;

  std::cout << "Generating keys...\n";
  auto keys = generate_keys(num_keys, prefix_heavy);

  Stack<std::string, 16UL> stack;

  stress_insert(stack, keys);

  std::cout << "sink=" << g_sink << "\n";
  return 0;
}
