#ifndef LRU_CACHE_HPP
#define LRU_CACHE_HPP

#include <cstddef>
#include <functional>
#include <list>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace quiz
{
  namespace util
  {
    template < class Key, class Value, class Hash = std::hash< Key > >
    class LruCache
    {
    private:
      using Items = std::list< std::pair< Key, Value > >;
      using ListIterator = typename Items::iterator;
      using Index = std::unordered_map< Key, ListIterator, Hash >;

      static constexpr bool NOTHROW_MOVE =
        std::is_nothrow_move_constructible_v< Index > && std::is_nothrow_move_constructible_v< Items >;
      static constexpr bool NOTHROW_SWAP = std::is_nothrow_swappable_v< Index > && std::is_nothrow_swappable_v< Items >;
      static constexpr bool NOTHROW_ERASE =
        std::is_nothrow_invocable_v< const Hash &, const Key & > &&
        noexcept(static_cast< bool >(std::declval< const Key & >() == std::declval< const Key & >()));

    public:
      explicit LruCache(size_t capacity): capacity_(capacity)
      {
      }

      LruCache(const LruCache &other):
        capacity_(other.capacity_), index_(0, other.index_.hash_function()), items_(other.items_)
      {
        for (auto it = items_.begin(); it != items_.end(); ++it)
        {
          index_.emplace(it->first, it);
        }
      }

      LruCache &operator=(const LruCache &other)
      {
        if (this != &other)
        {
          LruCache copy(other);
          swap(copy);
        }
        return *this;
      }

      LruCache(LruCache &&other) noexcept(NOTHROW_MOVE):
        capacity_(other.capacity_), index_(std::move(other.index_)), items_(std::move(other.items_))
      {
        other.clear();
      }

      LruCache &operator=(LruCache &&other) noexcept(NOTHROW_MOVE && NOTHROW_SWAP)
      {
        if (this != &other)
        {
          LruCache moved(std::move(other));
          swap(moved);
        }
        return *this;
      }

      void put(const Key &key, const Value &value)
      {
        if (capacity_ == 0)
        {
          return;
        }

        const auto FOUND = index_.find(key);
        if (FOUND != index_.end())
        {
          FOUND->second->second = value;
          moveToFront(FOUND->second);
          return;
        }

        Items pending;
        pending.emplace_front(key, value);
        if constexpr (!NOTHROW_ERASE)
        {
          if (items_.size() == capacity_)
          {
            evictLeastRecent();
          }
        }

        index_.emplace(pending.front().first, pending.begin());
        items_.splice(items_.begin(), pending, pending.begin());
        if (items_.size() > capacity_)
        {
          evictLeastRecent();
        }
      }

      std::optional< Value > get(const Key &key)
      {
        const auto FOUND = index_.find(key);
        if (FOUND == index_.end())
        {
          return std::nullopt;
        }
        moveToFront(FOUND->second);
        return FOUND->second->second;
      }

      bool contains(const Key &key) const
      {
        return index_.find(key) != index_.end();
      }

      size_t size() const
      {
        return items_.size();
      }

      void clear() noexcept
      {
        index_.clear();
        items_.clear();
      }

    private:
      void moveToFront(ListIterator position) noexcept
      {
        items_.splice(items_.begin(), items_, position);
      }

      void evictLeastRecent() noexcept(NOTHROW_ERASE)
      {
        if (!items_.empty())
        {
          index_.erase(items_.back().first);
          items_.pop_back();
        }
      }

      void swap(LruCache &other) noexcept(NOTHROW_SWAP)
      {
        index_.swap(other.index_);
        items_.swap(other.items_);
        using std::swap;
        swap(capacity_, other.capacity_);
      }

      size_t capacity_;
      Index index_;
      Items items_;
    };
  }
}

#endif
