#include "trie.hpp"

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

static void stress_insert(Trie& trie, const std::vector<std::string>& keys)
{
  measure_elapsed("insert bulk", [&]
  {
    for (const auto& k : keys)
    {
      trie.insert(k);
    }
  });
}

static void stress_contains(Trie& trie,
                            const std::vector<std::string>& keys,
                            std::size_t queries,
                            std::uint64_t seed = 0xBADC0DEULL)
{
  std::mt19937_64 rng(seed);
  std::uniform_int_distribution<std::size_t> pick(0, keys.size() - 1);

  measure_elapsed("contains mixed", [&]
  {
    std::uint64_t hits = 0;

    for (std::size_t i = 0; i < queries; i++)
    {
      if ((i & 1u) == 0u)
      {
        hits += trie.contains(keys[pick(rng)]) ? 1u : 0u;
      }
      else
      {
        const std::string miss = make_random_key(rng, 8, 32);
        hits += trie.contains(miss) ? 1u : 0u;
      }
    }

    g_sink ^= hits;
  });
}

static void stress_prefix(Trie& trie,
                          const std::vector<std::string>& keys,
                          std::size_t queries,
                          std::uint64_t seed = 0x12345678ULL)
{
  std::mt19937_64 rng(seed);
  std::uniform_int_distribution<std::size_t> pick(0, keys.size() - 1);
  std::uniform_int_distribution<std::size_t> prefix_len(1, 12);

  measure_elapsed("has_prefix mixed", [&]
  {
    std::uint64_t hits = 0;

    for (std::size_t i = 0; i < queries; ++i)
    {
      const std::string& base = keys[pick(rng)];
      const std::size_t n = std::min(prefix_len(rng), base.size());
      hits += trie.has_prefix(std::string_view(base.data(), n)) ? 1u : 0u;
    }

    g_sink ^= (hits << 1);
  });
}

static void stress_reads_multithread(Trie& trie,
                                     const std::vector<std::string>& keys,
                                     std::size_t total_queries,
                                     unsigned num_threads)
{
  if (num_threads == 0) num_threads = 1;

  measure_elapsed("contains (mt)", [&]
  {
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    const std::size_t per_thread = total_queries / num_threads;

    for (unsigned t = 0; t < num_threads; t++)
    {
      threads.emplace_back([&, t]
      {
        std::mt19937_64 rng(0xABC000ULL + t);
        std::uniform_int_distribution<std::size_t> pick(0, keys.size() - 1);

        std::uint64_t local_hits = 0;
        for (std::size_t i = 0; i < per_thread; i++)
        {
          local_hits += trie.contains(keys[pick(rng)]) ? 1u : 0u;
        }

        g_sink ^= (local_hits + t);
      });
    }

    for (auto& th : threads) th.join();
  });
}

int main(void)
{
  const std::size_t num_keys     = 200'000;
  const std::size_t num_queries  = 1'000'000;
  const bool        prefix_heavy = true;

  std::cout << "Generating keys...\n";
  auto keys = generate_keys(num_keys, prefix_heavy);

  Trie trie;

  stress_insert(trie, keys);

  stress_contains(trie, keys, num_queries);
  stress_prefix(trie, keys, num_queries);

  stress_reads_multithread(trie, keys, num_queries, std::thread::hardware_concurrency());

  std::cout << "sink=" << g_sink << "\n";
  return 0;
}
