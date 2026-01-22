#include "trie.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

inline std::size_t Trie::idx(unsigned char c) noexcept
{
  return static_cast<std::size_t>(c);
}

Trie::Trie() noexcept : m_root(std::make_unique<Node>()) {}

void Trie::insert(std::string_view key) noexcept
{
  Node* n = m_root.get();

  for (unsigned char uc : key)
  {
    auto& slot = n->children[idx(uc)];

    if (slot == nullptr)
    {
      slot = std::make_unique<Node>();
    }

    n = slot.get();
  }

  n->is_end = true;
}

bool Trie::contains(std::string_view key) const noexcept
{
  const Node* n = m_root.get();

  for (unsigned char uc : key)
  {
    const auto& slot = n->children[idx(uc)];

    if (slot == nullptr)
    {
      return false;
    }

    n = slot.get();
  }

  return n->is_end;
}

bool Trie::has_prefix(std::string_view prefix) const noexcept
{
  const Node* node = m_root.get();

  for (unsigned char uc : prefix)
  {
    const auto& slot = node->children[idx(uc)];

    if (slot == nullptr)
    {
      return false;
    }

    node = slot.get();
  }

  return true;
}

void Trie::clear(void) noexcept
{
  m_root = std::make_unique<Node>();
}
