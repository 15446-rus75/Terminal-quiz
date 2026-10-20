#include <algorithm>
#include <memory>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "util/lru_cache.hpp"
#include <boost/test/unit_test.hpp>

namespace
{
  struct CollisionHash
  {
    size_t operator()(int) const
    {
      return 0;
    }
  };

  struct CopyFailure
  {
    int value;
    std::shared_ptr< bool > fail;

    CopyFailure(int input, std::shared_ptr< bool > flag): value(input), fail(std::move(flag))
    {
    }

    CopyFailure(const CopyFailure &other): value(other.value), fail(other.fail)
    {
      if (*fail)
      {
        throw std::runtime_error("Copy failed");
      }
    }

    CopyFailure &operator=(const CopyFailure &) = default;
  };
}

using quiz::util::LruCache;

BOOST_AUTO_TEST_SUITE(lru_cache)

BOOST_AUTO_TEST_CASE(empty_and_missing_key)
{
  LruCache< int, std::string > cache(2);
  BOOST_TEST(cache.size() == 0U);
  BOOST_TEST(!cache.contains(1));
  BOOST_TEST(!cache.get(1).has_value());
}

BOOST_AUTO_TEST_CASE(put_and_replace_value)
{
  LruCache< int, std::string > cache(2);
  cache.put(1, "first");
  cache.put(1, "updated");
  BOOST_TEST(cache.size() == 1U);
  BOOST_REQUIRE(cache.get(1).has_value());
  BOOST_TEST(*cache.get(1) == "updated");
}

BOOST_AUTO_TEST_CASE(read_updates_recency)
{
  LruCache< int, int > cache(2);
  cache.put(1, 10);
  cache.put(2, 20);
  BOOST_REQUIRE(cache.get(1).has_value());
  cache.put(3, 30);
  BOOST_TEST(cache.contains(1));
  BOOST_TEST(!cache.contains(2));
  BOOST_TEST(cache.contains(3));
}

BOOST_AUTO_TEST_CASE(contains_and_failed_get_do_not_update_recency)
{
  LruCache< int, int > cache(2);
  cache.put(1, 10);
  cache.put(2, 20);
  BOOST_TEST(cache.contains(1));
  BOOST_TEST(!cache.get(99).has_value());
  cache.put(3, 30);
  BOOST_TEST(!cache.contains(1));
  BOOST_TEST(cache.contains(2));
}

BOOST_AUTO_TEST_CASE(replacement_updates_recency)
{
  LruCache< int, int > cache(2);
  cache.put(1, 10);
  cache.put(2, 20);
  cache.put(1, 11);
  cache.put(3, 30);
  BOOST_TEST(cache.size() == 2U);
  BOOST_TEST(!cache.contains(2));
  BOOST_REQUIRE(cache.get(1).has_value());
  BOOST_TEST(*cache.get(1) == 11);
}

BOOST_AUTO_TEST_CASE(zero_capacity_and_single_entry)
{
  LruCache< int, int > disabled(0);
  disabled.put(1, 1);
  BOOST_TEST(disabled.size() == 0U);
  BOOST_TEST(!disabled.get(1).has_value());
  LruCache< int, int > single(1);
  single.put(1, 1);
  single.put(2, 2);
  BOOST_TEST(single.size() == 1U);
  BOOST_TEST(!single.contains(1));
  BOOST_TEST(single.contains(2));
}

BOOST_AUTO_TEST_CASE(clear_and_reuse)
{
  LruCache< int, int > cache(2);
  cache.put(1, 1);
  cache.put(2, 2);
  cache.clear();
  cache.clear();
  BOOST_TEST(cache.size() == 0U);
  BOOST_TEST(!cache.get(1).has_value());
  cache.put(3, 3);
  BOOST_TEST(cache.contains(3));
}

BOOST_AUTO_TEST_CASE(custom_hash_and_collisions)
{
  LruCache< int, std::string, CollisionHash > cache(2);
  cache.put(1, "one");
  cache.put(2, "two");
  const auto FIRST = cache.get(1);
  BOOST_REQUIRE(FIRST.has_value());
  BOOST_TEST(*FIRST == "one");
  cache.put(3, "three");
  BOOST_TEST(cache.contains(1));
  BOOST_TEST(!cache.contains(2));
  const auto THIRD = cache.get(3);
  BOOST_REQUIRE(THIRD.has_value());
  BOOST_TEST(*THIRD == "three");
}

BOOST_AUTO_TEST_CASE(copy_survives_original_destruction)
{
  std::optional< LruCache< int, int > > copied;
  {
    LruCache< int, int > original(2);
    original.put(1, 10);
    original.put(2, 20);
    copied.emplace(original);
    original.clear();
  }
  BOOST_REQUIRE(copied->get(1).has_value());
  copied->put(3, 30);
  BOOST_TEST(!copied->contains(2));
  BOOST_TEST(copied->contains(1));
}

BOOST_AUTO_TEST_CASE(copy_assignment_replaces_capacity_and_contents)
{
  LruCache< int, int > original(2);
  original.put(1, 10);
  original.put(2, 20);
  LruCache< int, int > assigned(1);
  assigned.put(99, 99);
  assigned = original;
  original.clear();
  assigned.put(3, 30);
  BOOST_TEST(assigned.size() == 2U);
  BOOST_TEST(!assigned.contains(1));
  BOOST_TEST(assigned.contains(2));
  BOOST_TEST(assigned.contains(3));
  BOOST_TEST(!assigned.contains(99));
}

BOOST_AUTO_TEST_CASE(move_constructor_and_moved_from_reuse)
{
  LruCache< int, int > original(2);
  original.put(1, 10);
  original.put(2, 20);
  LruCache< int, int > moved(std::move(original));
  BOOST_REQUIRE(moved.get(1).has_value());
  moved.put(3, 30);
  BOOST_TEST(!moved.contains(2));
  BOOST_TEST(original.size() == 0U);
  original.put(5, 50);
  BOOST_TEST(original.contains(5));
}

BOOST_AUTO_TEST_CASE(move_assignment)
{
  LruCache< int, int > original(2);
  original.put(1, 10);
  original.put(2, 20);
  LruCache< int, int > moved(1);
  moved.put(99, 99);
  moved = std::move(original);
  moved.put(3, 30);
  BOOST_TEST(moved.size() == 2U);
  BOOST_TEST(!moved.contains(1));
  BOOST_TEST(!moved.contains(99));
  BOOST_TEST(moved.contains(2));
  BOOST_TEST(original.size() == 0U);
}

BOOST_AUTO_TEST_CASE(failed_value_copy_leaves_cache_usable)
{
  auto fail = std::make_shared< bool >(false);
  LruCache< int, CopyFailure > cache(1);
  const CopyFailure FIRST(10, fail);
  const CopyFailure SECOND(20, fail);
  cache.put(1, FIRST);
  *fail = true;
  BOOST_CHECK_THROW(cache.put(2, SECOND), std::runtime_error);
  *fail = false;
  BOOST_TEST(cache.size() == 1U);
  BOOST_REQUIRE(cache.get(1).has_value());
  BOOST_TEST(cache.get(1)->value == 10);
  cache.put(2, SECOND);
  BOOST_TEST(!cache.contains(1));
  BOOST_TEST(cache.get(2)->value == 20);
}

BOOST_AUTO_TEST_CASE(random_operations_match_independent_reference)
{
  constexpr size_t CAPACITY = 7;
  LruCache< int, int > cache(CAPACITY);
  std::vector< std::pair< int, int > > reference;
  std::mt19937 random(20261016);
  for (size_t step = 0; step < 5000; ++step)
  {
    const int KEY = static_cast< int >(random() % 17);
    const unsigned int OPERATION = random() % 3;
    const auto FOUND = std::find_if(reference.begin(), reference.end(),
                                    [KEY](const auto &ENTRY)
                                    {
                                      return ENTRY.first == KEY;
                                    });
    if (OPERATION == 0)
    {
      const int VALUE = static_cast< int >(step);
      cache.put(KEY, VALUE);
      if (FOUND != reference.end())
      {
        reference.erase(FOUND);
      }
      reference.emplace_back(KEY, VALUE);
      if (reference.size() > CAPACITY)
      {
        reference.erase(reference.begin());
      }
    }
    else if (OPERATION == 1)
    {
      const auto ACTUAL = cache.get(KEY);
      BOOST_TEST(ACTUAL.has_value() == (FOUND != reference.end()));
      if (FOUND != reference.end())
      {
        BOOST_TEST(*ACTUAL == FOUND->second);
        const auto ENTRY = *FOUND;
        reference.erase(FOUND);
        reference.push_back(ENTRY);
      }
    }
    else
    {
      BOOST_TEST(cache.contains(KEY) == (FOUND != reference.end()));
    }
    BOOST_TEST(cache.size() == reference.size());
    for (int candidate = 0; candidate < 17; ++candidate)
    {
      const bool EXPECTED = std::any_of(reference.begin(), reference.end(),
                                        [candidate](const auto &ENTRY)
                                        {
                                          return ENTRY.first == candidate;
                                        });
      BOOST_TEST(cache.contains(candidate) == EXPECTED);
    }
  }
}

BOOST_AUTO_TEST_SUITE_END()
