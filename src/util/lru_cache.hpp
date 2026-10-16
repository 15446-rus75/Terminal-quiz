#ifndef LRU_CACHE_HPP
#define LRU_CACHE_HPP

#include <cstddef>
#include <functional>
#include <list>
#include <optional>
#include <unordered_map>
#include <utility>

namespace quiz
{
  namespace util
  {
    template < class Key, class Value, class Hash = std::hash< Key > >
    class LruCache
    {
    public:
      explicit LruCache(size_t capacity);
      LruCache(const LruCache &other);
      LruCache &operator=(const LruCache &other);
      LruCache(LruCache &&other);
      LruCache &operator=(LruCache &&other);
      void put(const Key &key, const Value &value);
      std::optional< Value > get(const Key &key);
      bool contains(const Key &key) const;
      size_t size() const;
      void clear();

    private:
      using ListIterator = typename std::list< std::pair< Key, Value > >::iterator;

      void moveToFront(const Key &key);
      void evictLeastRecent();
      void swap(LruCache &other);

      size_t capacity_;
      std::list< std::pair< Key, Value > > items_;
      std::unordered_map< Key, ListIterator, Hash > index_;
    };
  }
}

#include "lru_cache.tpp"

#endif
