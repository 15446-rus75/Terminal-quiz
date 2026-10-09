#ifndef IFAVORITE_REPOSITORY_HPP
#define IFAVORITE_REPOSITORY_HPP

#include "../core/question.hpp"
#include <cstdint>
#include <vector>

namespace quiz
{
  namespace db
  {
    class IFavoriteRepository
    {
    public:
      virtual ~IFavoriteRepository() = default;
      virtual void add(size_t user_id, size_t question_id) = 0;
      virtual void remove(size_t user_id, size_t question_id) = 0;
      virtual std::vector< core::Question > findByUserId(size_t user_id) = 0;
      virtual bool exists(size_t user_id, size_t question_id) = 0;
    };
  }
}

#endif
