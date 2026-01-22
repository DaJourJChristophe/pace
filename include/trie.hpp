#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <string_view>

class Trie
{
protected:
  static constexpr std::size_t kAlphabet = 256UL;

  struct Node final
  {
    bool is_end{false};
    std::array<std::unique_ptr<Node>, kAlphabet> children{};

    Node() noexcept = default;
  };

  std::unique_ptr<Node> m_root;

  [[nodiscard]] static inline std::size_t idx(unsigned char c) noexcept;

public:
  Trie() noexcept;

  void insert(std::string_view key) noexcept;

  [[nodiscard]] bool contains(std::string_view key) const noexcept;

  [[nodiscard]] bool has_prefix(std::string_view prefix) const noexcept;

  void clear(void) noexcept;
};
