#ifndef FAVORITE_SERVICE_HPP
#define FAVORITE_SERVICE_HPP

#include "../core/question.hpp"
#include "../db/i_favorite_repository.hpp"
#include <cstdint>
#include <memory>
#include <vector>

namespace quiz
{
  namespace service
  {
    class FavoriteService
    {
    public:
      explicit FavoriteService(std::shared_ptr< db::IFavoriteRepository > favorite_repository);
      void add(size_t user_id, size_t question_id);
      void remove(size_t user_id, size_t question_id);
      std::vector< core::Question > getAll(size_t user_id);
      bool isFavorite(size_t user_id, size_t question_id);

    private:
      std::shared_ptr< db::IFavoriteRepository > favorite_repository_;
    };
  }
}

#endif
