#include "map.hpp"

#include <array>
#include <cassert>
#include <string>

template <typename K, typename V, std::size_t N>
class MockMap : public Map<K, V, N>
{
public:
  static constexpr int EMPTY = static_cast<int>(
    MockMap<K, V, N>::BucketState::EMPTY);

  MockMap() noexcept : Map<K, V, N>() {}

  const std::array<typename MockMap<K, V, N>::Bucket, N>& get_slots(void) const noexcept
  {
    return this->m_slots;
  }
};

void test_map_init(void)
{
  using MockMapDef = MockMap<std::string, std::string, 4UL>;
  MockMapDef map;
  const auto& slots = map.get_slots();

  for (std::uint64_t i = 0UL; i < 4UL; i++)
  {
    const auto& bucket = slots[i];

    assert(static_cast<int>(bucket.state) == MockMapDef::EMPTY);
    assert(bucket.key                     == "");
    assert(bucket.val                     == "");
  }
}

void test_map_set(void)
{
  using MockMapDef = MockMap<std::string, std::string, 4UL>;
  MockMapDef map;

  assert(map.set("foo", "bar") == 0);
  assert(map.set("fragile", "tar") == 0);
  assert(map.set("Hello, World!", "How are you today?") == 0);
  assert(map.set("Hello, Again!", "I-am-well-and-you?") == 0);

  assert(map.set("toy", "car") == (-1));
  assert(map.set("boy", "far") == (-1));
  assert(map.set("coi", "star") == (-1));
  assert(map.set("ran", "maps") == (-1));
}

void test_map_get(void)
{
  using MockMapDef = MockMap<std::string, std::string, 4UL>;
  MockMapDef map;

  assert(map.set("foo", "bar")                          == 0);
  assert(map.set("fragile", "tar")                      == 0);
  assert(map.set("Hello, World!", "How are you today?") == 0);
  assert(map.set("Hello, Again!", "I-am-well-and-you?") == 0);

  assert(map.set("toy", "car")  == (-1));
  assert(map.set("boy", "far")  == (-1));
  assert(map.set("coi", "star") == (-1));
  assert(map.set("ran", "maps") == (-1));

  std::string out = "";

  assert(map.get(out, "foo")           == 0 && out == "bar");
  assert(map.get(out, "fragile")       == 0 && out == "tar");
  assert(map.get(out, "Hello, World!") == 0 && out == "How are you today?");
  assert(map.get(out, "Hello, Again!") == 0 && out == "I-am-well-and-you?");

  out = "";

  assert(map.get(out, "toy") == (-1) && out == "");
  assert(map.get(out, "boy") == (-1) && out == "");
  assert(map.get(out, "coi") == (-1) && out == "");
  assert(map.get(out, "ran") == (-1) && out == "");
}

void test_map_del(void)
{
  using MockMapDef = MockMap<std::string, std::string, 4UL>;
  MockMapDef map;

  assert(map.set("foo", "bar") == 0);
  assert(map.set("fragile", "tar") == 0);
  assert(map.set("Hello, World!", "How are you today?") == 0);
  assert(map.set("Hello, Again!", "I-am-well-and-you?") == 0);

  std::string out = "";

  assert(map.get(out, "foo") == 0 && out == "bar");
  assert(map.get(out, "fragile") == 0 && out == "tar");
  assert(map.get(out, "Hello, World!") == 0 && out == "How are you today?");
  assert(map.get(out, "Hello, Again!") == 0 && out == "I-am-well-and-you?");

  assert(map.del("Hello, World!") == 0);

  out = "";

  assert(map.get(out, "Hello, World!") == (-1) && out == "");
}

int main(void)
{
  test_map_init();
  test_map_set();
  test_map_get();
  test_map_del();

  return 0;
}
