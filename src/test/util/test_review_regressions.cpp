#include <functional>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "util/lru_cache.hpp"
#include <boost/test/unit_test.hpp>

namespace
{
  struct ThrowingHash
  {
    static inline int rejected_key = -1;
    static inline int counted_key = -1;
    static inline int count = 0;
    static inline int rejected_count = -1;

    size_t operator()(int key) const
    {
      if (key == rejected_key || (key == counted_key && ++count == rejected_count))
      {
        throw std::runtime_error("Hash failed");
      }
      return std::hash< int >{}(key);
    }
  };

  struct CopyKey
  {
    static inline int copies_left = -1;
    int value;

    explicit CopyKey(int input): value(input)
    {
    }

    CopyKey(const CopyKey &other): value(other.value)
    {
      if (copies_left == 0)
      {
        throw std::runtime_error("Key copy failed");
      }
      if (copies_left > 0)
      {
        --copies_left;
      }
    }

    bool operator==(const CopyKey &other) const noexcept
    {
      return value == other.value;
    }
  };

  struct CopyKeyHash
  {
    size_t operator()(const CopyKey &key) const noexcept
    {
      return std::hash< int >{}(key.value);
    }
  };

  struct ThrowingHashCopy
  {
    static inline bool fail = false;

    ThrowingHashCopy() = default;

    ThrowingHashCopy(const ThrowingHashCopy &)
    {
      if (fail)
      {
        throw std::runtime_error("Hash copy failed");
      }
    }

    ThrowingHashCopy &operator=(const ThrowingHashCopy &) = default;

    size_t operator()(int key) const noexcept
    {
      return std::hash< int >{}(key);
    }
  };

  struct ThrowingEqualKey
  {
    static inline bool fail = false;
    int value;

    bool operator==(const ThrowingEqualKey &other) const
    {
      if (fail && value == other.value)
      {
        throw std::runtime_error("Comparison failed");
      }
      return value == other.value;
    }
  };

  struct EqualKeyHash
  {
    size_t operator()(const ThrowingEqualKey &) const noexcept
    {
      return 0;
    }
  };

  struct ResetFailures
  {
    ~ResetFailures()
    {
      ThrowingHash::rejected_key = -1;
      ThrowingHash::counted_key = -1;
      ThrowingHash::count = 0;
      ThrowingHash::rejected_count = -1;
      CopyKey::copies_left = -1;
      ThrowingHashCopy::fail = false;
      ThrowingEqualKey::fail = false;
    }
  };

  using DefaultCache = quiz::util::LruCache< int, int >;
  using CopyHashCache = quiz::util::LruCache< int, int, ThrowingHashCopy >;
  static_assert(std::is_nothrow_move_constructible_v< DefaultCache >);
  static_assert(std::is_nothrow_move_assignable_v< DefaultCache >);
  static_assert(!std::is_nothrow_move_constructible_v< CopyHashCache >);
  static_assert(!std::is_nothrow_move_assignable_v< CopyHashCache >);
}

BOOST_FIXTURE_TEST_SUITE(review_regressions, ResetFailures)

BOOST_AUTO_TEST_CASE(throwing_hash_during_eviction_preserves_existing_entries)
{
  quiz::util::LruCache< int, int, ThrowingHash > cache(2);
  cache.put(1, 10);
  cache.put(2, 20);
  ThrowingHash::rejected_key = 1;
  BOOST_CHECK_THROW(cache.put(3, 30), std::runtime_error);
  ThrowingHash::rejected_key = -1;
  BOOST_TEST(cache.size() == 2U);
  BOOST_TEST(cache.contains(1));
  BOOST_TEST(cache.contains(2));
  BOOST_TEST(!cache.contains(3));
  cache.put(3, 30);
  BOOST_TEST(!cache.contains(1));
  BOOST_TEST(cache.contains(3));
}

BOOST_AUTO_TEST_CASE(throwing_hash_during_insertion_keeps_cache_consistent)
{
  quiz::util::LruCache< int, int, ThrowingHash > cache(2);
  cache.put(1, 10);
  cache.put(2, 20);
  ThrowingHash::counted_key = 3;
  ThrowingHash::rejected_count = 2;
  BOOST_CHECK_THROW(cache.put(3, 30), std::runtime_error);
  ThrowingHash::counted_key = -1;
  BOOST_TEST(cache.size() <= 2U);
  BOOST_TEST(cache.contains(2));
  BOOST_TEST(!cache.contains(3));
  cache.put(3, 30);
  cache.put(4, 40);
  BOOST_TEST(cache.size() == 2U);
  BOOST_TEST(cache.contains(3));
  BOOST_TEST(cache.contains(4));
}

BOOST_AUTO_TEST_CASE(failed_index_key_copy_does_not_evict_for_nothrow_hash)
{
  quiz::util::LruCache< CopyKey, int, CopyKeyHash > cache(2);
  const CopyKey FIRST(1);
  const CopyKey SECOND(2);
  const CopyKey THIRD(3);
  cache.put(FIRST, 10);
  cache.put(SECOND, 20);
  CopyKey::copies_left = 1;
  BOOST_CHECK_THROW(cache.put(THIRD, 30), std::runtime_error);
  CopyKey::copies_left = -1;
  BOOST_TEST(cache.size() == 2U);
  BOOST_TEST(cache.contains(FIRST));
  BOOST_TEST(cache.contains(SECOND));
  BOOST_TEST(!cache.contains(THIRD));
  cache.put(THIRD, 30);
  BOOST_TEST(!cache.contains(FIRST));
  BOOST_TEST(cache.contains(THIRD));
}

BOOST_AUTO_TEST_CASE(failed_hash_copy_during_move_preserves_source_list)
{
  CopyHashCache original(2);
  original.put(1, 10);
  original.put(2, 20);
  ThrowingHashCopy::fail = true;
  BOOST_CHECK_THROW(CopyHashCache(std::move(original)), std::runtime_error);
  ThrowingHashCopy::fail = false;
  BOOST_TEST(original.size() == 2U);
  BOOST_REQUIRE(original.get(1).has_value());
  BOOST_TEST(*original.get(1) == 10);
  original.put(3, 30);
  BOOST_TEST(!original.contains(2));
}

BOOST_AUTO_TEST_CASE(throwing_equality_does_not_terminate_eviction)
{
  quiz::util::LruCache< ThrowingEqualKey, int, EqualKeyHash > cache(2);
  const ThrowingEqualKey FIRST{1};
  const ThrowingEqualKey SECOND{2};
  const ThrowingEqualKey THIRD{3};
  cache.put(FIRST, 10);
  cache.put(SECOND, 20);
  ThrowingEqualKey::fail = true;
  BOOST_CHECK_THROW(cache.put(THIRD, 30), std::runtime_error);
  ThrowingEqualKey::fail = false;
  BOOST_TEST(cache.size() == 2U);
  BOOST_TEST(cache.contains(FIRST));
  BOOST_TEST(cache.contains(SECOND));
  BOOST_TEST(!cache.contains(THIRD));
}

BOOST_AUTO_TEST_SUITE_END()
