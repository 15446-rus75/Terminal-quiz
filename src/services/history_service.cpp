#include "history_service.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "../core/attempt.hpp"
#include "../core/question.hpp"
#include "../db/iattempt_repository.hpp"
#include "../db/iquestion_repository.hpp"

namespace
{
  class QuestionLoader
  {
  public:
    explicit QuestionLoader(quiz::db::IQuestionRepository &repository): repository_(repository)
    {
    }

    quiz::core::Question operator()(const quiz::core::AttemptAnswer &answer) const
    {
      std::optional< quiz::core::Question > question = repository_.findById(answer.question_id);
      if (!question.has_value())
      {
        throw std::runtime_error("Question referenced by the attempt not found");
      }
      return std::move(*question);
    }

  private:
    quiz::db::IQuestionRepository &repository_;
  };
}

quiz::service::HistoryService::HistoryService(std::shared_ptr< db::IAttemptRepository > attempt_repository,
                                              std::shared_ptr< db::IQuestionRepository > question_repository):
  attempt_repository_(std::move(attempt_repository)),
  question_repository_(std::move(question_repository))
{
  if (attempt_repository_ == nullptr)
  {
    throw std::invalid_argument("Attempt repository must not be null");
  }
  if (question_repository_ == nullptr)
  {
    throw std::invalid_argument("Question repository must not be null");
  }
}

std::vector< quiz::core::Attempt > quiz::service::HistoryService::recent(size_t user_id, size_t limit)
{
  if (limit == 0)
  {
    return {};
  }
  return attempt_repository_->findByUserId(user_id, limit);
}

std::optional< quiz::service::AttemptDetails > quiz::service::HistoryService::details(size_t attempt_id)
{
  std::optional< core::Attempt > attempt = attempt_repository_->findById(attempt_id);
  if (!attempt.has_value())
  {
    return std::nullopt;
  }

  AttemptDetails result{};
  result.questions.reserve(attempt->answers.size());
  std::transform(attempt->answers.begin(), attempt->answers.end(), std::back_inserter(result.questions),
                 QuestionLoader(*question_repository_));
  result.attempt = std::move(*attempt);
  return result;
}
