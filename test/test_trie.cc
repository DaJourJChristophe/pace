#include "trie.hpp"

#include <cassert>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

namespace
{
  template <std::size_t BucketCapacity = 64UL>
  class MockTrie : public HATTrie<BucketCapacity>
  {
  public:
    MockTrie() noexcept : HATTrie<BucketCapacity>() {}

    std::size_t get_max_children(void) const noexcept
    {
      return this->kAlphabet;
    }

    typename HATTrie<BucketCapacity>::Node* get_root(void) const noexcept
    {
      return this->m_root.get();
    }
  };

  static inline void insert_words(MockTrie<>& trie) noexcept
  {
    trie.insert("foo");
    trie.insert("far");
    trie.insert("bar");
    trie.insert("car");
  }
} // namespace

void test_trie_root(void)
{
  MockTrie<> trie;
  const auto root = trie.get_root();

  assert(root         != nullptr);
  assert(root->is_end == false  );

  const std::size_t n = trie.get_max_children();

  for (std::uint64_t i = 0UL; i < n; i++)
  {
    assert(root->children[i] == nullptr);
  }
}

void test_trie_insert_and_contains(void)
{
  MockTrie<> trie;

  insert_words(trie);

  assert(trie.contains("foo") == true);
  assert(trie.contains("far") == true);
  assert(trie.contains("bar") == true);
  assert(trie.contains("car") == true);

  assert(trie.contains("fo")  == false);
  assert(trie.contains("ca")  == false);
  assert(trie.contains("f")   == false);
  assert(trie.contains("c")   == false);

  assert(trie.contains("tar") == false);
  assert(trie.contains("and") == false);
  assert(trie.contains("man") == false);
  assert(trie.contains("van") == false);
}

void test_trie_has_prefix(void)
{
  MockTrie<> trie;

  insert_words(trie);

  assert(trie.has_prefix("fo")  == true);
  assert(trie.has_prefix("b")   == true);
  assert(trie.has_prefix("car") == true);
  assert(trie.has_prefix("fa")  == true);

  assert(trie.has_prefix("f")   == true);
  assert(trie.has_prefix("c")   == true);

  assert(trie.has_prefix("ko")  == false);
  assert(trie.has_prefix("fl")  == false);
  assert(trie.has_prefix("baz") == false);
  assert(trie.has_prefix("ch")  == false);
}

void test_trie_clear(void)
{
  MockTrie<> trie;

  insert_words(trie);

  assert(trie.contains("foo") == true);
  assert(trie.has_prefix("f") == true);

  trie.clear();

  const auto root = trie.get_root();

  assert(root         != nullptr);
  assert(root->is_end == false  );

  const std::size_t n = trie.get_max_children();

  for (std::uint64_t i = 0UL; i < n; i++)
  {
    assert(root->children[i] == nullptr);
  }

  assert(trie.contains("foo") == false);
  assert(trie.contains("far") == false);
  assert(trie.contains("bar") == false);
  assert(trie.contains("car") == false);

  assert(trie.has_prefix("f")  == false);
  assert(trie.has_prefix("b")  == false);
  assert(trie.has_prefix("c")  == false);
  assert(trie.has_prefix("")   == false);
}

void test_trie_bucket_stress_promote(void)
{
  // Force bucket overflow so promotion/splitting happens.
  // Use a small bucket to make the test fast/deterministic.
  MockTrie<8UL> trie;

  // All share "a" prefix so they should collide into the same hot node early.
  for (std::uint64_t i = 0UL; i < 64UL; i++)
  {
    std::string s = "a";
    s += std::to_string(static_cast<unsigned long long>(i));
    trie.insert(s);
  }

  // Must still be query-correct after promotions.
  for (std::uint64_t i = 0UL; i < 64UL; i++)
  {
    std::string s = "a";
    s += std::to_string(static_cast<unsigned long long>(i));
    assert(trie.contains(s) == true);
  }

  assert(trie.has_prefix("a") == true);
  assert(trie.has_prefix("b") == false);
}

int main(void)
{
  test_trie_root();
  test_trie_insert_and_contains();
  test_trie_has_prefix();
  test_trie_clear();
  test_trie_bucket_stress_promote();

  return EXIT_SUCCESS;
}
