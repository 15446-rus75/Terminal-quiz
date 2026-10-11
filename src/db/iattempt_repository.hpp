#ifndef IATTEMPT_REPOSITORY_HPP
#define IATTEMPT_REPOSITORY_HPP

#include <chrono>
#include <cstddef>
#include <optional>
#include <vector>

#include "../core/attempt.hpp"

namespace quiz
{
  namespace db
  {
    class IAttemptRepository
    {
    public:
      virtual ~IAttemptRepository() = default;
      virtual size_t save(const core::Attempt &attempt) = 0;
      virtual void saveAnswers(size_t attempt_id, const std::vector< core::AttemptAnswer > &answers) = 0;
      virtual void finish(size_t attempt_id, size_t score, std::chrono::system_clock::time_point finished_at) = 0;
      virtual std::vector< core::Attempt > findByUserId(size_t user_id, size_t limit) = 0;
      virtual std::optional< core::Attempt > findById(size_t id) = 0;
    };
  }
}

#endif
