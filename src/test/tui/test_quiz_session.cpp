#include <boost/test/unit_test.hpp>
#include "tui/quiz_session.hpp"
#include "tui/answer_arranger.hpp"
#include "services/favorite_service.hpp"
#include "core/enums.hpp"
#include "test_support.hpp"

namespace
{
  quiz::core::Question makeSessionQuestion(size_t id)
  {
    quiz::core::Question question;
    question.id = id;
    question.category = quiz::core::Category::GENERAL_KNOWLEDGE;
    question.difficulty = quiz::core::Difficulty::EASY;
    question.type = quiz::core::QuestionType::MULTIPLE;
    question.text = "Question " + std::to_string(id);
    question.correct_answer = "Paris";
    question.incorrect_answers = {"London"};
    return question;
  }
}

BOOST_AUTO_TEST_SUITE(quiz_session_tests)

BOOST_AUTO_TEST_CASE(session_throws_on_null_terminal)
{
  auto fav_repo = std::make_shared< test::MockFavoriteRepository >();
  auto fav_service = std::make_shared< quiz::service::FavoriteService >(fav_repo);
  auto arranger = std::make_shared< quiz::tui::RandomAnswerArranger >(42);

  BOOST_CHECK_THROW(quiz::tui::QuizSession(nullptr, fav_service, arranger, 1), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(session_throws_on_null_favorite_service)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  auto arranger = std::make_shared< quiz::tui::RandomAnswerArranger >(42);

  BOOST_CHECK_THROW(quiz::tui::QuizSession(terminal, nullptr, arranger, 1), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(session_throws_on_null_arranger)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  auto fav_repo = std::make_shared< test::MockFavoriteRepository >();
  auto fav_service = std::make_shared< quiz::service::FavoriteService >(fav_repo);

  BOOST_CHECK_THROW(quiz::tui::QuizSession(terminal, fav_service, nullptr, 1), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(session_answers_question_correctly)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"1"});

  auto fav_repo = std::make_shared< test::MockFavoriteRepository >();
  auto fav_service = std::make_shared< quiz::service::FavoriteService >(fav_repo);
  auto arranger = std::make_shared< quiz::tui::RandomAnswerArranger >(42);

  quiz::tui::QuizSession session(terminal, fav_service, arranger, 1);
  const auto outcome = session.run({makeSessionQuestion(1)});

  BOOST_CHECK_EQUAL(outcome.answers.size(), 1);
  BOOST_CHECK(!outcome.stopped_early);
}

BOOST_AUTO_TEST_CASE(session_skip_command)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"/skip", "1"});

  auto fav_repo = std::make_shared< test::MockFavoriteRepository >();
  auto fav_service = std::make_shared< quiz::service::FavoriteService >(fav_repo);
  auto arranger = std::make_shared< quiz::tui::RandomAnswerArranger >(42);

  quiz::tui::QuizSession session(terminal, fav_service, arranger, 1);
  const auto outcome = session.run({makeSessionQuestion(1), makeSessionQuestion(2)});

  BOOST_CHECK_EQUAL(outcome.answers.size(), 1);
  BOOST_CHECK(!outcome.stopped_early);
}

BOOST_AUTO_TEST_CASE(session_stop_command)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"/stop"});

  auto fav_repo = std::make_shared< test::MockFavoriteRepository >();
  auto fav_service = std::make_shared< quiz::service::FavoriteService >(fav_repo);
  auto arranger = std::make_shared< quiz::tui::RandomAnswerArranger >(42);

  quiz::tui::QuizSession session(terminal, fav_service, arranger, 1);
  const auto outcome = session.run({makeSessionQuestion(1)});

  BOOST_CHECK_EQUAL(outcome.answers.size(), 0);
  BOOST_CHECK(outcome.stopped_early);
}

BOOST_AUTO_TEST_SUITE_END()
