#include "services/favorite_service.hpp"
#include "services/quiz_service.hpp"
#include "test_support.hpp"
#include "tui/answer_arranger.hpp"
#include "tui/quiz_runner.hpp"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(quiz_runner_tests)

BOOST_AUTO_TEST_CASE(runner_throws_on_null_terminal)
{
  auto att_repo = std::make_shared< test::MockAttemptRepository >();
  auto quiz_service = std::make_shared< quiz::service::QuizService >(att_repo);
  auto fav_repo = std::make_shared< test::MockFavoriteRepository >();
  auto fav_service = std::make_shared< quiz::service::FavoriteService >(fav_repo);
  auto arranger = std::make_shared< quiz::tui::RandomAnswerArranger >(42);

  BOOST_CHECK_THROW(quiz::tui::QuizRunner(nullptr, quiz_service, fav_service, arranger, 1), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(runner_throws_on_null_quiz_service)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  auto fav_repo = std::make_shared< test::MockFavoriteRepository >();
  auto fav_service = std::make_shared< quiz::service::FavoriteService >(fav_repo);
  auto arranger = std::make_shared< quiz::tui::RandomAnswerArranger >(42);

  BOOST_CHECK_THROW(quiz::tui::QuizRunner(terminal, nullptr, fav_service, arranger, 1), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(runner_empty_questions_shows_message)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  auto att_repo = std::make_shared< test::MockAttemptRepository >();
  auto quiz_service = std::make_shared< quiz::service::QuizService >(att_repo);
  auto fav_repo = std::make_shared< test::MockFavoriteRepository >();
  auto fav_service = std::make_shared< quiz::service::FavoriteService >(fav_repo);
  auto arranger = std::make_shared< quiz::tui::RandomAnswerArranger >(42);

  quiz::tui::QuizRunner runner(terminal, quiz_service, fav_service, arranger, 1);
  runner.play({});

  BOOST_CHECK(terminal->getOutput().find("no questions") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()
