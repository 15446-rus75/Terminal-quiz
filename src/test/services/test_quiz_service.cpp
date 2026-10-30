#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

#include "core/attempt.hpp"
#include "core/question.hpp"
#include "db/iattempt_repository.hpp"
#include "services/quiz_service.hpp"
#include <boost/test/unit_test.hpp>

namespace core = quiz::core;
namespace db = quiz::db;
namespace service = quiz::service;

namespace
{
  class MockAttemptRepository: public db::IAttemptRepository
  {
  public:
    size_t save(const core::Attempt &att) override
    {
      saved_attempt = att;
      saved_attempt->id = 1;
      return 1;
    }

    void saveAnswers(size_t attempt_id, const std::vector< core::AttemptAnswer > &answers) override
    {
      saved_answers = answers;
      if (saved_attempt && saved_attempt->id == attempt_id)
      {
        saved_attempt->answers = answers;
      }
    }

    void finish(size_t attempt_id, size_t score, std::chrono::system_clock::time_point finished_at) override
    {
      if (saved_attempt && saved_attempt->id == attempt_id)
      {
        saved_attempt->score = score;
        saved_attempt->finished_at = finished_at;
      }
    }

    std::optional< core::Attempt > findById(size_t id) override
    {
      if (saved_attempt && saved_attempt->id == id)
      {
        return saved_attempt;
      }
      return std::nullopt;
    }

    std::vector< core::Attempt > findByUserId(size_t, size_t) override
    {
      return {};
    }

    std::optional< core::Attempt > saved_attempt;
    std::vector< core::AttemptAnswer > saved_answers;
  };

  core::Question makeQuestion(size_t id, const std::string &correct)
  {
    core::Question q;
    q.id = id;
    q.correct_answer = correct;
    return q;
  }
}

BOOST_AUTO_TEST_SUITE(quiz_service_constructor)

BOOST_AUTO_TEST_CASE(null_repository_throws)
{
  BOOST_CHECK_THROW(service::QuizService(nullptr), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(quiz_service_workflow)

BOOST_AUTO_TEST_CASE(start_quiz_empty_questions_throws)
{
  auto repo = std::make_shared< MockAttemptRepository >();
  service::QuizService service(repo);

  BOOST_CHECK_THROW(service.startQuiz(1, {}), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(start_submit_finish_quiz_flow)
{
  auto repo = std::make_shared< MockAttemptRepository >();
  service::QuizService service(repo);

  const auto Q1 = makeQuestion(10, "Paris");
  const auto Q2 = makeQuestion(20, "42");
  const std::vector< core::Question > QUESTIONS = {Q1, Q2};

  const size_t ATTEMPT_ID = service.startQuiz(100, QUESTIONS);
  BOOST_TEST(ATTEMPT_ID == 1U);

  const std::vector< service::QuizAnswer > ANSWERS = {
    service::QuizAnswer{.question_id = 10, .user_answer = "paris"},
    service::QuizAnswer{.question_id = 20, .user_answer = "wrong"}
  };

  service.submitAnswers(ATTEMPT_ID, QUESTIONS, ANSWERS);

  BOOST_TEST(repo->saved_answers.size() == 2U);
  BOOST_TEST(repo->saved_answers[0].is_correct);
  BOOST_TEST(!repo->saved_answers[1].is_correct);

  const auto RESULT = service.finishQuiz(ATTEMPT_ID);
  BOOST_TEST(RESULT.attempt_id == ATTEMPT_ID);
  BOOST_TEST(RESULT.score == 1U);
  BOOST_TEST(RESULT.total == 2U);
  BOOST_TEST(RESULT.accuracy == 0.5);
}

BOOST_AUTO_TEST_CASE(submit_invalid_question_throws)
{
  auto repo = std::make_shared< MockAttemptRepository >();
  service::QuizService service(repo);

  const auto Q1 = makeQuestion(10, "Paris");
  const size_t ATTEMPT_ID = service.startQuiz(1, {Q1});

  const std::vector< service::QuizAnswer > ANSWERS = {
    service::QuizAnswer{.question_id = 999, .user_answer = "Paris"}
  };

  BOOST_CHECK_THROW(service.submitAnswers(ATTEMPT_ID, {Q1}, ANSWERS), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(finish_non_existing_attempt_throws)
{
  auto repo = std::make_shared< MockAttemptRepository >();
  service::QuizService service(repo);

  BOOST_CHECK_THROW(service.finishQuiz(999), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()
