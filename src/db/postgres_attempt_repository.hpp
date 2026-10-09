#ifndef POSTGRES_ATTEMPT_REPOSITORY_HPP
#define POSTGRES_ATTEMPT_REPOSITORY_HPP

#include "iattempt_repository.hpp"
#include "idatabase.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace quiz
{
  namespace db
  {
    class PostgresAttemptRepository: public IAttemptRepository
    {
    public:
      explicit PostgresAttemptRepository(std::shared_ptr< IDatabase > database);
      size_t save(const core::Attempt &attempt) override;
      std::vector< core::Attempt > findByUserId(size_t user_id, size_t limit) override;
      std::optional< core::Attempt > findById(size_t id) override;

    private:
      void saveAnswers(size_t attempt_id, const std::vector< core::AttemptAnswer > &answers);

      std::shared_ptr< IDatabase > database_;
    };
  }
}

#endif
