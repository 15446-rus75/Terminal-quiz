#ifndef POSTGRES_ATTEMPT_REPOSITORY_HPP
#define POSTGRES_ATTEMPT_REPOSITORY_HPP

#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "../core/attempt.hpp"
#include "iattempt_repository.hpp"
#include "idatabase.hpp"

namespace quiz
{
  namespace db
  {
    class PostgresAttemptRepository: public IAttemptRepository
    {
    public:
      explicit PostgresAttemptRepository(std::shared_ptr< IDatabase > database);
      size_t save(const core::Attempt &attempt) override;
      void saveAnswers(size_t attempt_id, const std::vector< core::AttemptAnswer > &answers) override;
      void finish(size_t attempt_id, size_t score, std::chrono::system_clock::time_point finished_at) override;
      std::vector< core::Attempt > findByUserId(size_t user_id, size_t limit) override;
      std::optional< core::Attempt > findById(size_t id) override;

    private:
      std::shared_ptr< IDatabase > database_;
    };
  }
}

#endif
