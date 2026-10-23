#ifndef HISTORY_SERVICE_HPP
#define HISTORY_SERVICE_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "../core/attempt.hpp"
#include "../core/question.hpp"
#include "../db/iattempt_repository.hpp"
#include "../db/iquestion_repository.hpp"

namespace quiz
{
  namespace service
  {
    struct AttemptDetails
    {
      core::Attempt attempt;
      std::vector< core::Question > questions;
    };

    class HistoryService
    {
    public:
      HistoryService(std::shared_ptr< db::IAttemptRepository > attempt_repository,
                     std::shared_ptr< db::IQuestionRepository > question_repository);
      std::vector< core::Attempt > recent(size_t user_id, size_t limit);
      std::optional< AttemptDetails > details(size_t attempt_id);

    private:
      std::shared_ptr< db::IAttemptRepository > attempt_repository_;
      std::shared_ptr< db::IQuestionRepository > question_repository_;
    };
  }
}

#endif
