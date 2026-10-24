#include "custom_test_service.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../core/custom_test.hpp"
#include "../core/question.hpp"
#include "../db/icustom_test_repository.hpp"
#include "../db/iquestion_repository.hpp"

namespace
{
  class QuestionLoader
  {
  public:
    explicit QuestionLoader(quiz::db::IQuestionRepository &repository): repository_(repository)
    {
    }

    quiz::core::Question operator()(size_t question_id) const
    {
      std::optional< quiz::core::Question > question = repository_.findById(question_id);
      if (!question.has_value())
      {
        throw std::invalid_argument("Question not found");
      }
      return std::move(*question);
    }

  private:
    quiz::db::IQuestionRepository &repository_;
  };
}

quiz::service::CustomTestService::CustomTestService(std::shared_ptr< db::ICustomTestRepository > custom_test_repository,
                                                    std::shared_ptr< db::IQuestionRepository > question_repository):
  custom_test_repository_(std::move(custom_test_repository)),
  question_repository_(std::move(question_repository))
{
  if (custom_test_repository_ == nullptr)
  {
    throw std::invalid_argument("Custom test repository must not be null");
  }
  if (question_repository_ == nullptr)
  {
    throw std::invalid_argument("Question repository must not be null");
  }
}

size_t quiz::service::CustomTestService::create(size_t user_id, const std::string &name,
                                                const std::vector< size_t > &question_ids)
{
  if (name.empty())
  {
    throw std::invalid_argument("Custom test name must not be empty");
  }
  if (question_ids.empty())
  {
    throw std::invalid_argument("Custom test must contain at least one question");
  }

  std::vector< size_t > sorted_ids = question_ids;
  std::sort(sorted_ids.begin(), sorted_ids.end());
  if (std::adjacent_find(sorted_ids.begin(), sorted_ids.end()) != sorted_ids.end())
  {
    throw std::invalid_argument("Custom test contains duplicate questions");
  }

  std::vector< core::Question > questions;
  questions.reserve(question_ids.size());
  std::transform(question_ids.begin(), question_ids.end(), std::back_inserter(questions),
                 QuestionLoader(*question_repository_));

  core::CustomTest test{};
  test.user_id = user_id;
  test.name = name;
  test.created_at = std::chrono::system_clock::now();
  test.question_ids = question_ids;
  return custom_test_repository_->save(test);
}

std::optional< quiz::core::CustomTest > quiz::service::CustomTestService::findById(size_t id)
{
  return custom_test_repository_->findById(id);
}

std::vector< quiz::core::CustomTest > quiz::service::CustomTestService::findByUserId(size_t user_id)
{
  return custom_test_repository_->findByUserId(user_id);
}

void quiz::service::CustomTestService::remove(size_t id)
{
  custom_test_repository_->remove(id);
}

std::vector< quiz::core::Question > quiz::service::CustomTestService::loadQuestions(size_t test_id)
{
  const std::optional< core::CustomTest > TEST = custom_test_repository_->findById(test_id);
  if (!TEST.has_value())
  {
    throw std::invalid_argument("Custom test not found");
  }

  std::vector< core::Question > questions;
  questions.reserve(TEST->question_ids.size());
  std::transform(TEST->question_ids.begin(), TEST->question_ids.end(), std::back_inserter(questions),
                 QuestionLoader(*question_repository_));
  return questions;
}
