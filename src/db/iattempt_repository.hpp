#ifndef IATTEMPT_REPOSITORY_HPP
#define IATTEMPT_REPOSITORY_HPP

#include "../core/attempt.hpp"
#include <cstdint>
#include <vector>

namespace quiz
{
  namespace db
  {
    class IAttemptRepository
    {
    public:
      virtual ~IAttemptRepository() = default;
      virtual size_t save(const core::Attempt &attempt) = 0;
      virtual std::vector< core::Attempt > findByUserId(size_t user_id, size_t limit) = 0;
      virtual std::optional< core::Attempt > findById(size_t id) = 0;
    };
  }
}

#endif
