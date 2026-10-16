#ifndef LRU_CACHE_TPP
#define LRU_CACHE_TPP

namespace quiz
{
  namespace util
  {
    template < class Key, class Value, class Hash >
    LruCache< Key, Value, Hash >::LruCache(size_t capacity): capacity_(capacity)
    {
    }

    template < class Key, class Value, class Hash >
    LruCache< Key, Value, Hash >::LruCache(const LruCache &other):
      capacity_(other.capacity_), items_(other.items_), index_(0, other.index_.hash_function())
    {
      for (auto it = items_.begin(); it != items_.end(); ++it)
      {
        index_.emplace(it->first, it);
      }
    }

    template < class Key, class Value, class Hash >
    LruCache< Key, Value, Hash > &LruCache< Key, Value, Hash >::operator=(const LruCache &other)
    {
      if (this != &other)
      {
        LruCache copy(other);
        swap(copy);
      }
      return *this;
    }

    template < class Key, class Value, class Hash >
    LruCache< Key, Value, Hash >::LruCache(LruCache &&other):
      capacity_(other.capacity_), items_(std::move(other.items_)), index_(std::move(other.index_))
    {
      other.clear();
    }

    template < class Key, class Value, class Hash >
    LruCache< Key, Value, Hash > &LruCache< Key, Value, Hash >::operator=(LruCache &&other)
    {
      if (this != &other)
      {
        LruCache moved(std::move(other));
        swap(moved);
      }
      return *this;
    }

    template < class Key, class Value, class Hash >
    void LruCache< Key, Value, Hash >::put(const Key &key, const Value &value)
    {
      if (capacity_ == 0)
      {
        return;
      }

      const auto FOUND = index_.find(key);
      if (FOUND != index_.end())
      {
        FOUND->second->second = value;
        moveToFront(key);
        return;
      }

      items_.emplace_front(key, value);
      const auto INSERTED = [this]()
      {
        try
        {
          return index_.emplace(items_.front().first, items_.begin()).first;
        }
        catch (...)
        {
          items_.pop_front();
          throw;
        }
      }();

      try
      {
        if (items_.size() > capacity_)
        {
          evictLeastRecent();
        }
      }
      catch (...)
      {
        index_.erase(INSERTED);
        items_.pop_front();
        throw;
      }
    }

    template < class Key, class Value, class Hash >
    std::optional< Value > LruCache< Key, Value, Hash >::get(const Key &key)
    {
      const auto FOUND = index_.find(key);
      if (FOUND == index_.end())
      {
        return std::nullopt;
      }
      moveToFront(key);
      return FOUND->second->second;
    }

    template < class Key, class Value, class Hash >
    bool LruCache< Key, Value, Hash >::contains(const Key &key) const
    {
      return index_.find(key) != index_.end();
    }

    template < class Key, class Value, class Hash >
    size_t LruCache< Key, Value, Hash >::size() const
    {
      return items_.size();
    }

    template < class Key, class Value, class Hash >
    void LruCache< Key, Value, Hash >::clear()
    {
      index_.clear();
      items_.clear();
    }

    template < class Key, class Value, class Hash >
    void LruCache< Key, Value, Hash >::moveToFront(const Key &key)
    {
      const auto FOUND = index_.find(key);
      if (FOUND != index_.end())
      {
        items_.splice(items_.begin(), items_, FOUND->second);
      }
    }

    template < class Key, class Value, class Hash >
    void LruCache< Key, Value, Hash >::evictLeastRecent()
    {
      if (!items_.empty())
      {
        index_.erase(items_.back().first);
        items_.pop_back();
      }
    }

    template < class Key, class Value, class Hash >
    void LruCache< Key, Value, Hash >::swap(LruCache &other)
    {
      using std::swap;
      swap(capacity_, other.capacity_);
      items_.swap(other.items_);
      index_.swap(other.index_);
    }
  }
}

#endif
