#pragma once

#include "map.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

template <std::size_t BucketCapacity = 64UL>
class HATTrie
{
protected:
  static constexpr std::size_t kAlphabet = 256UL;

  // We reuse your Map as the “bucket” dictionary at leaves.
  // Value is a byte just to mark presence.
  struct BucketMap final : public Map<std::string, std::uint8_t, BucketCapacity>
  {
    using Base        = Map<std::string, std::uint8_t, BucketCapacity>;
    using Bucket      = typename Base::Bucket;
    using BucketState = typename Base::BucketState;

    using Base::m_slots;

    BucketMap() noexcept : Base() {}

    [[nodiscard]] bool contains(std::string_view key) noexcept
    {
      std::uint8_t out = 0U;
      return (this->get(out, std::string(key)) == 0);
    }

    [[nodiscard]] bool has_prefix(std::string_view prefix) const noexcept
    {
      for (std::uint64_t i = 0UL; i < BucketCapacity; i++)
      {
        const Bucket& b = m_slots[i];

        if (static_cast<std::uint8_t>(b.state) != 1U)
        {
          continue;
        }

        const std::string& k = b.key;

        if (k.size() < prefix.size())
        {
          continue;
        }

        // prefix compare (avoid <algorithm>)
        if (std::memcmp(k.data(), prefix.data(), prefix.size()) == 0)
        {
          return true;
        }
      }

      return false;
    }
  };

  struct Node final
  {
    bool                                         is_end{false};
    std::unique_ptr<BucketMap>                   bucket{};
    std::array<std::unique_ptr<Node>, kAlphabet> children{};

    Node() noexcept : is_end(false), bucket(std::make_unique<BucketMap>()), children() {}

    [[nodiscard]] bool is_bucket(void) const noexcept
    {
      return (bucket != nullptr);
    }
  };

  std::unique_ptr<Node> m_root;

  [[nodiscard]] static inline std::size_t idx(unsigned char c) noexcept
  {
    return static_cast<std::size_t>(c);
  }

  static inline void m_promote_bucket(Node& node, std::size_t depth) noexcept
  {
    // Turn a bucket-node into an internal node and redistribute keys.
    std::unique_ptr<BucketMap> old = std::move(node.bucket);
    node.bucket = nullptr;

    // Re-insert every occupied key from the old bucket.
    for (std::uint64_t i = 0UL; i < BucketCapacity; i++)
    {
      auto& b = old->m_slots[i];

      if (static_cast<std::uint8_t>(b.state) != 1U)
      {
        continue;
      }

      const std::string& key = b.key;

      // Key ends exactly at this prefix depth.
      if (depth >= key.size())
      {
        node.is_end = true;
        continue;
      }

      const unsigned char uc = static_cast<unsigned char>(key[depth]);
      auto& child = node.children[idx(uc)];

      if (child == nullptr)
      {
        child = std::make_unique<Node>();
      }

      // Reinsert into child (may cause future splits during normal insert).
      (void)child->bucket->set(key, 1U);
    }

    // Note: old is destroyed here; its strings are copied into child buckets.
  }

public:
  HATTrie() noexcept : m_root(std::make_unique<Node>()) {}

  void clear(void) noexcept
  {
    m_root = std::make_unique<Node>();
  }

  void insert(std::string_view key) noexcept
  {
    Node* node = m_root.get();
    std::size_t depth = 0UL;

    // We may need to retry after splitting a full bucket.
    for (;;)
    {
      if (node->is_bucket())
      {
        // Insert full key into this bucket.
        // If it overflows, split/promote this node and retry at same depth.
        const int rc = node->bucket->set(std::string(key), 1U);

        if (rc == 0)
        {
          return;
        }

        // Bucket full: promote and redistribute using next character at "depth".
        m_promote_bucket(*node, depth);
        continue; // retry insert
      }

      // Internal node path
      if (depth >= key.size())
      {
        node->is_end = true;
        return;
      }

      const unsigned char uc = static_cast<unsigned char>(key[depth]);
      auto& child = node->children[idx(uc)];

      if (child == nullptr)
      {
        child = std::make_unique<Node>();
      }

      node = child.get();
      ++depth;
    }
  }

  [[nodiscard]] bool contains(std::string_view key) const noexcept
  {
    const Node* node = m_root.get();
    std::size_t depth = 0UL;

    for (;;)
    {
      if (node == nullptr)
      {
        return false;
      }

      if (node->is_bucket())
      {
        return node->bucket->contains(key);
      }

      if (depth >= key.size())
      {
        return node->is_end;
      }

      const unsigned char uc = static_cast<unsigned char>(key[depth]);
      node = node->children[idx(uc)].get();
      ++depth;
    }
  }

  [[nodiscard]] bool has_prefix(std::string_view prefix) const noexcept
  {
    const Node* node = m_root.get();
    std::size_t depth = 0UL;

    for (;;)
    {
      if (node == nullptr)
      {
        return false;
      }

      if (node->is_bucket())
      {
        return node->bucket->has_prefix(prefix);
      }

      if (depth >= prefix.size())
      {
        if (node->is_end)
        {
          return true;
        }

        for (std::uint64_t i = 0UL; i < kAlphabet; i++)
        {
          if (node->children[i] != nullptr)
          {
            return true;
          }
        }

        return false;
      }

      const unsigned char uc = static_cast<unsigned char>(prefix[depth]);
      node = node->children[idx(uc)].get();
      ++depth;
    }
  }

  [[nodiscard]] bool matches_prefix(std::string_view s) const noexcept
  {
    const Node* node = m_root.get();
    std::size_t depth = 0UL;

    for (;;)
    {
      if (node == nullptr)
      {
        return false;
      }

      if (node->is_end)
      {
        return true;
      }

      if (node->is_bucket())
      {
        for (std::uint64_t i = 0UL; i < BucketCapacity; i++)
        {
          const auto& b = node->bucket->m_slots[i];

          if (static_cast<std::uint8_t>(b.state) != 1U)
          {
            continue;
          }

          const std::string& k = b.key;

          if (k.size() == 0UL)
          {
            continue;
          }

          if (k.size() > s.size())
          {
            continue;
          }

          if (std::memcmp(s.data(), k.data(), k.size()) == 0)
          {
            return true;
          }
        }

        return false;
      }

      if (depth >= s.size())
      {
        return false;
      }

      const unsigned char uc = static_cast<unsigned char>(s[depth]);
      node = node->children[idx(uc)].get();
      ++depth;
    }
  }

  [[nodiscard]] bool matches_substring(std::string_view s) const noexcept
  {
    for (std::size_t i = 0UL; i < s.size(); i++)
    {
      if (matches_prefix(s.substr(i)))
      {
        return true;
      }
    }

    return false;
  }
};
