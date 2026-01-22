#include "trie.hpp"

#include <cassert>
#include <cstdlib>
#include <memory>

namespace
{
  class MockTrie : public Trie
  {
  public:
    MockTrie() noexcept : Trie() {}

    std::size_t get_max_children(void) const noexcept
    {
      return kAlphabet;
    }

    Node* get_root(void) const noexcept
    {
      return m_root.get();
    }
  };
} // namespace

void test_trie_root(void)
{
  MockTrie trie;
  const auto root = trie.get_root();

  assert(root         != nullptr);
  assert(root->is_end == false  );

  const std::size_t n = trie.get_max_children();

  for (std::uint64_t i = 0UL; i < n; i++)
  {
    assert(root->children[i] == nullptr);
  }
}

void test_trie_insert(void)
{
  MockTrie trie;
  
  trie.insert("foo");
  trie.insert("far");
  trie.insert("bar");
  trie.insert("car");

  const auto root = trie.get_root();

  assert(root         != nullptr);
  assert(root->is_end == false  );

  // Test the first level of the tree.

  assert(root->children[ 98] != nullptr);
  assert(root->children[ 99] != nullptr);
  assert(root->children[102] != nullptr);


  // Test the second level of the tree.

  auto child = root->children[98].get();

  assert(child != nullptr && child->children[ 97] != nullptr);

  child = root->children[ 99].get();

  assert(child != nullptr && child->children[ 97] != nullptr);

  child = root->children[102].get();

  assert(child != nullptr && child->children[ 97] != nullptr);
  assert(child != nullptr && child->children[111] != nullptr);


  // Test the third level of the tree.

  child =  root->children[ 98].get();
  child = child->children[ 97].get();

  assert(child != nullptr && child->children[114] != nullptr);


  child =  root->children[ 99].get();
  child = child->children[ 97].get();

  assert(child != nullptr && child->children[114] != nullptr);


  child =  root->children[102].get();
  child = child->children[ 97].get();

  assert(child != nullptr && child->children[114] != nullptr);

  child =  root->children[102].get();
  child = child->children[111].get();

  assert(child != nullptr && child->children[111] != nullptr);
}

void test_trie_clear(void)
{
  MockTrie trie;

  trie.insert("foo");
  trie.insert("far");
  trie.insert("bar");
  trie.insert("car");

  trie.clear();

  const auto root = trie.get_root();

  assert(root         != nullptr);
  assert(root->is_end == false  );

  const std::size_t n = trie.get_max_children();

  for (std::uint64_t i = 0UL; i < n; i++)
  {
    assert(root->children[i] == nullptr);
  }
}

void test_trie_contains(void)
{
  MockTrie trie;

  trie.insert("foo");
  trie.insert("far");
  trie.insert("bar");
  trie.insert("car");

  assert(trie.contains("foo") == true);
  assert(trie.contains("far") == true);
  assert(trie.contains("bar") == true);
  assert(trie.contains("car") == true);

  assert(trie.contains("tar") == false);
  assert(trie.contains("and") == false);
  assert(trie.contains("man") == false);
  assert(trie.contains("van") == false);
}

void test_trie_has_prefix(void)
{
  MockTrie trie;

  trie.insert("foo");
  trie.insert("far");
  trie.insert("bar");
  trie.insert("car");

  assert(trie.has_prefix("fo")  == true);
  assert(trie.has_prefix("b")   == true);
  assert(trie.has_prefix("car") == true);
  assert(trie.has_prefix("fa")  == true);

  assert(trie.has_prefix("ko")  == false);
  assert(trie.has_prefix("fl")  == false);
  assert(trie.has_prefix("baz") == false);
  assert(trie.has_prefix("ch")  == false);
}

int main(void)
{
  test_trie_root();
  test_trie_insert();
  test_trie_clear();
  test_trie_contains();
  test_trie_has_prefix();

  return EXIT_SUCCESS;
}
