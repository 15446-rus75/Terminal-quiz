#ifndef LRU_CACHE_HPP
#define LRU_CACHE_HPP

#include <cstddef>
#include <list>
#include <optional>
#include <unordered_map>

namespace quiz
{
  namespace util
  {
    template < class Key, class Value > class LruCache
    {
    public:
      explicit LruCache(std::size_t capacity) void put(const Key &key, const Value &value);
      std::optional< Value > get(const Key &key);
      bool contains(const Key &key) const;
      size_t size() const;
      void clear();

    private:
      using ListIterator = typename std::list< std::pair< Key, Value > >::iterator;
      void moveToFront(const Key &key);
      void evictLeastRecent();

      size_t capacity_;
      std::list< std::pair< Key, Value > > items_;
      std::unordered_map< Key, ListIterator > index_;
    };
  }
}

#endif
