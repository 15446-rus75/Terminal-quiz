#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

#include "core/attempt.hpp"
#include "core/question.hpp"
#include "db/iattempt_repository.hpp"
#include "db/iquestion_repository.hpp"
#include "services/history_service.hpp"
#include <boost/test/unit_test.hpp>

namespace core = quiz::core;
namespace db = quiz::db;
namespace service = quiz::service;

namespace
{
  class MockAttemptRepository: public db::IAttemptRepository
  {
  public:
    size_t save(const core::Attempt &) override
    {
      return 0;
    }

    void saveAnswers(size_t, const std::vector< core::AttemptAnswer > &) override
    {
    }

    void finish(size_t, size_t, std::chrono::system_clock::time_point) override
    {
    }

    std::optional< core::Attempt > findById(size_t id) override
    {
      for (const auto &att: attempts)
      {
        if (att.id == id)
        {
          return att;
        }
      }
      return std::nullopt;
    }

    std::vector< core::Attempt > findByUserId(size_t user_id, size_t limit) override
    {
      std::vector< core::Attempt > result;
      for (const auto &att: attempts)
      {
        if (att.user_id == user_id)
        {
          result.push_back(att);
          if (result.size() == limit)
          {
            break;
          }
        }
      }
      return result;
    }

    std::vector< core::Attempt > attempts;
  };

  class MockQuestionRepository: public db::IQuestionRepository
  {
  public:
    size_t save(const core::Question &) override
    {
      return 0;
    }

    std::vector< size_t > saveBatch(const std::vector< core::Question > &) override
    {
      return {};
    }

    std::optional< core::Question > findById(size_t id) override
    {
      for (const auto &q: questions)
      {
        if (q.id == id)
        {
          return q;
        }
      }
      return std::nullopt;
    }

    std::vector< core::Question > findByFilter(core::Category, core::Difficulty, size_t) override
    {
      return {};
    }

    std::vector< core::Question > searchByText(const std::string &, core::Category, core::Difficulty, size_t) override
    {
      return {};
    }

    std::vector< core::Question > questions;
  };
}

BOOST_AUTO_TEST_SUITE(history_service_constructor)

BOOST_AUTO_TEST_CASE(null_repositories_throw)
{
  auto attempt_repo = std::make_shared< MockAttemptRepository >();
  auto q_repo = std::make_shared< MockQuestionRepository >();

  BOOST_CHECK_THROW(service::HistoryService(nullptr, q_repo), std::invalid_argument);
  BOOST_CHECK_THROW(service::HistoryService(attempt_repo, nullptr), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(history_service_recent)

BOOST_AUTO_TEST_CASE(zero_limit_returns_empty)
{
  auto attempt_repo = std::make_shared< MockAttemptRepository >();
  auto q_repo = std::make_shared< MockQuestionRepository >();

  service::HistoryService service(attempt_repo, q_repo);
  BOOST_TEST(service.recent(1, 0).empty());
}

BOOST_AUTO_TEST_CASE(returns_recent_attempts)
{
  auto attempt_repo = std::make_shared< MockAttemptRepository >();
  auto q_repo = std::make_shared< MockQuestionRepository >();

  core::Attempt att1;
  att1.id = 1;
  att1.user_id = 10;
  core::Attempt att2;
  att2.id = 2;
  att2.user_id = 10;
  attempt_repo->attempts = {att1, att2};

  service::HistoryService service(attempt_repo, q_repo);
  const auto RECENT = service.recent(10, 1);
  BOOST_TEST(RECENT.size() == 1U);
  BOOST_TEST(RECENT[0].id == 1U);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(history_service_details)

BOOST_AUTO_TEST_CASE(non_existing_attempt_returns_nullopt)
{
  auto attempt_repo = std::make_shared< MockAttemptRepository >();
  auto q_repo = std::make_shared< MockQuestionRepository >();

  service::HistoryService service(attempt_repo, q_repo);
  BOOST_TEST(!service.details(999).has_value());
}

BOOST_AUTO_TEST_CASE(existing_attempt_returns_details)
{
  auto attempt_repo = std::make_shared< MockAttemptRepository >();
  auto q_repo = std::make_shared< MockQuestionRepository >();

  core::Question q1;
  q1.id = 101;
  q_repo->questions = {q1};

  core::Attempt att;
  att.id = 1;
  att.answers = {core::AttemptAnswer{.id = 1, .attempt_id = 1, .question_id = 101, .user_answer = "Ans", .is_correct = true, .position = 0}};
  attempt_repo->attempts = {att};

  service::HistoryService service(attempt_repo, q_repo);
  const auto DETAILS = service.details(1);

  BOOST_TEST(DETAILS.has_value());
  BOOST_TEST(DETAILS->attempt.id == 1U);
  BOOST_TEST(DETAILS->questions.size() == 1U);
  BOOST_TEST(DETAILS->questions[0].id == 101U);
}

BOOST_AUTO_TEST_CASE(missing_referenced_question_throws)
{
  auto attempt_repo = std::make_shared< MockAttemptRepository >();
  auto q_repo = std::make_shared< MockQuestionRepository >();

  core::Attempt att;
  att.id = 1;
  att.answers = {core::AttemptAnswer{.id = 1, .attempt_id = 1, .question_id = 999, .user_answer = "Ans", .is_correct = true, .position = 0}};
  attempt_repo->attempts = {att};

  service::HistoryService service(attempt_repo, q_repo);
  BOOST_CHECK_THROW(service.details(1), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
