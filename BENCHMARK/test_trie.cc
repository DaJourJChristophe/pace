#include "trie.hpp"

#include <chrono>
#include <iostream>
#include <thread>

template <typename F>
void measure_elapsed(F&& callback)
{
  const auto start    = std::chrono::steady_clock::now();

  callback();

  const auto end      = std::chrono::steady_clock::now();
  const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  std::cout << "Elapsed time: " << duration.count() << " milliseconds\n";
}

void benchmark_trie_ctor(void)
{
  measure_elapsed([] {
    Trie trie; (void)trie;
  });
}

void benchmark_trie_insert(void)
{
  Trie trie;

  measure_elapsed([&trie] {
    trie.insert("foo");
  });
}

void benchmark_trie_clear(void)
{
  Trie trie;

  trie.insert("foo");
  trie.insert("far");
  trie.insert("bar");
  trie.insert("car");

  measure_elapsed([&trie] {
    trie.clear();
  });
}

void benchmark_trie_contains(void)
{
  Trie trie;

  trie.insert("foo");
  trie.insert("far");
  trie.insert("bar");
  trie.insert("car");

  measure_elapsed([&trie] {
    (void)trie.contains("foo");
  });
}

void benchmark_trie_has_prefix(void)
{
  Trie trie;

  trie.insert("foo");
  trie.insert("far");
  trie.insert("bar");
  trie.insert("car");

  measure_elapsed([&trie] {
    (void)trie.has_prefix("fo");
  });
}

int main(void)
{
  benchmark_trie_ctor();
  benchmark_trie_insert();
  benchmark_trie_clear();
  benchmark_trie_contains();
  benchmark_trie_has_prefix();

  return 0;
}
