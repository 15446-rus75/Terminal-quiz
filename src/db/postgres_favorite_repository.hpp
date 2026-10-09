#ifndef POSTGRES_FAVORITE_REPOSITORY_HPP
#define POSTGRES_FAVORITE_REPOSITORY_HPP

#include "idatabase.hpp"
#include "ifavorite_repository.hpp"
#include <cstdint>
#include <memory>
#include <vector>

namespace quiz
{
  namespace db
  {
    class PostgresFavoriteRepository: public IFavoriteRepository
    {
    public:
      explicit PostgresFavoriteRepository(std::shared_ptr< IDatabase > database);
      void add(size_t user_id, size_t question_id) override;
      void remove(size_t user_id, size_t question_id) override;
      std::vector< core::Question > findByUserId(size_t user_id) override;
      bool exists(size_t user_id, size_t question_id) override;

    private:
      std::shared_ptr< IDatabase > database_;
    };
  }
}

#endif
