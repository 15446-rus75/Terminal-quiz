#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "api/open_tdb_client.hpp"
#include "core/custom_test.hpp"
#include "core/enums.hpp"
#include "core/question.hpp"
#include "core/statistics.hpp"
#include "db/icustom_test_repository.hpp"
#include "db/istatistics_repository.hpp"
#include "services/custom_test_service.hpp"
#include "services/favorite_service.hpp"
#include "services/history_service.hpp"
#include "services/question_service.hpp"
#include "services/quiz_service.hpp"
#include "services/statistics_service.hpp"
#include "test_support.hpp"
#include "tui/answer_arranger.hpp"
#include "tui/custom_test_screen.hpp"
#include "tui/favorites_screen.hpp"
#include "tui/history_screen.hpp"
#include "tui/quiz_runner.hpp"
#include "tui/quiz_screen.hpp"
#include "tui/search_screen.hpp"
#include "tui/statistics_screen.hpp"
#include <boost/test/unit_test.hpp>

namespace core = quiz::core;
namespace db = quiz::db;
namespace api = quiz::api;
namespace service = quiz::service;
namespace tui = quiz::tui;

namespace
{
  class MockOpenTdbClient: public api::IOpenTdbClient
  {
  public:
    std::vector< core::Question > fetchQuestions(const api::OpenTdbRequest &) override
    {
      ++fetch_calls;
      return api_questions;
    }

    size_t fetch_calls = 0;
    std::vector< core::Question > api_questions;
  };

  class MockStatisticsRepository: public db::IStatisticsRepository
  {
  public:
    core::OverallStats getOverall(size_t) override
    {
      return overall;
    }

    std::vector< core::CategoryStats > getByCategory(size_t) override
    {
      return by_category;
    }

    std::vector< core::DifficultyStats > getByDifficulty(size_t) override
    {
      return by_difficulty;
    }

    core::OverallStats overall;
    std::vector< core::CategoryStats > by_category;
    std::vector< core::DifficultyStats > by_difficulty;
  };

  class MockCustomTestRepository: public db::ICustomTestRepository
  {
  public:
    size_t save(const core::CustomTest &test) override
    {
      auto stored = test;
      stored.id = ++next_id;
      tests.push_back(stored);
      return stored.id;
    }

    std::optional< core::CustomTest > findById(size_t id) override
    {
      for (const auto &test : tests)
      {
        if (test.id == id)
        {
          return test;
        }
      }
      return std::nullopt;
    }

    std::vector< core::CustomTest > findByUserId(size_t user_id) override
    {
      std::vector< core::CustomTest > result;
      for (const auto &test : tests)
      {
        if (test.user_id == user_id)
        {
          result.push_back(test);
        }
      }
      return result;
    }

    void remove(size_t id) override
    {
      std::erase_if(tests,
                    [id](const core::CustomTest &t)
                    {
                      return t.id == id;
                    });
    }

    size_t next_id = 0;
    std::vector< core::CustomTest > tests;
  };
}

BOOST_AUTO_TEST_SUITE(quiz_screen_tests)

BOOST_AUTO_TEST_CASE(throws_on_null_question_service)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  BOOST_CHECK_THROW(tui::QuizScreen(terminal, nullptr, nullptr), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(title_is_play_a_quiz)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  auto client = std::make_shared< MockOpenTdbClient >();
  auto q_repo = std::make_shared< test::MockQuestionRepository >();
  auto question_service = std::make_shared< service::QuestionService >(client, q_repo, 10);
  auto att_repo = std::make_shared< test::MockAttemptRepository >();
  auto quiz_service = std::make_shared< service::QuizService >(att_repo);
  auto fav_repo = std::make_shared< test::MockFavoriteRepository >();
  auto fav_service = std::make_shared< service::FavoriteService >(fav_repo);
  auto arranger = std::make_shared< tui::RandomAnswerArranger >(42);
  auto runner = std::make_shared< tui::QuizRunner >(terminal, quiz_service, fav_service, arranger, 1);

  tui::QuizScreen screen(terminal, question_service, runner);
  BOOST_CHECK_EQUAL(screen.title(), "Play a quiz");
}

BOOST_AUTO_TEST_CASE(back_on_parameters_exits_without_fetch)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"/back"});

  auto client = std::make_shared< MockOpenTdbClient >();
  auto q_repo = std::make_shared< test::MockQuestionRepository >();
  auto question_service = std::make_shared< service::QuestionService >(client, q_repo, 10);
  auto att_repo = std::make_shared< test::MockAttemptRepository >();
  auto quiz_service = std::make_shared< service::QuizService >(att_repo);
  auto fav_repo = std::make_shared< test::MockFavoriteRepository >();
  auto fav_service = std::make_shared< service::FavoriteService >(fav_repo);
  auto arranger = std::make_shared< tui::RandomAnswerArranger >(42);
  auto runner = std::make_shared< tui::QuizRunner >(terminal, quiz_service, fav_service, arranger, 1);

  tui::QuizScreen screen(terminal, question_service, runner);
  screen.run();

  BOOST_CHECK_EQUAL(client->fetch_calls, 0);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(history_screen_tests)

BOOST_AUTO_TEST_CASE(throws_on_null_service)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  BOOST_CHECK_THROW(tui::HistoryScreen(terminal, nullptr, 1), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(title_is_history)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  auto att_repo = std::make_shared< test::MockAttemptRepository >();
  auto q_repo = std::make_shared< test::MockQuestionRepository >();
  auto history_service = std::make_shared< service::HistoryService >(att_repo, q_repo);

  tui::HistoryScreen screen(terminal, history_service, 1);
  BOOST_CHECK_EQUAL(screen.title(), "History");
}

BOOST_AUTO_TEST_CASE(empty_history_shows_message_and_exits)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  auto att_repo = std::make_shared< test::MockAttemptRepository >();
  auto q_repo = std::make_shared< test::MockQuestionRepository >();
  auto history_service = std::make_shared< service::HistoryService >(att_repo, q_repo);

  tui::HistoryScreen screen(terminal, history_service, 1);
  screen.run();

  BOOST_CHECK(terminal->getOutput().find("No attempts yet.") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(back_on_details_exits)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"/back"});

  auto att_repo = std::make_shared< test::MockAttemptRepository >();
  core::Attempt att;
  att.id = 1;
  att.user_id = 1;
  att_repo->attempts_ = {att};

  auto q_repo = std::make_shared< test::MockQuestionRepository >();
  auto history_service = std::make_shared< service::HistoryService >(att_repo, q_repo);

  tui::HistoryScreen screen(terminal, history_service, 1);
  screen.run();

  BOOST_CHECK(terminal->getOutput().find("Attempt to view") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(favorites_screen_tests)

BOOST_AUTO_TEST_CASE(throws_on_null_dependencies)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  BOOST_CHECK_THROW(tui::FavoritesScreen(terminal, nullptr, nullptr, 1), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(title_is_favorites)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  auto fav_repo = std::make_shared< test::MockFavoriteRepository >();
  auto fav_service = std::make_shared< service::FavoriteService >(fav_repo);
  auto att_repo = std::make_shared< test::MockAttemptRepository >();
  auto quiz_service = std::make_shared< service::QuizService >(att_repo);
  auto arranger = std::make_shared< tui::RandomAnswerArranger >(42);
  auto runner = std::make_shared< tui::QuizRunner >(terminal, quiz_service, fav_service, arranger, 1);

  tui::FavoritesScreen screen(terminal, fav_service, runner, 1);
  BOOST_CHECK_EQUAL(screen.title(), "Favorites");
}

BOOST_AUTO_TEST_CASE(empty_favorites_shows_message_and_exits)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  auto fav_repo = std::make_shared< test::MockFavoriteRepository >();
  auto fav_service = std::make_shared< service::FavoriteService >(fav_repo);
  auto att_repo = std::make_shared< test::MockAttemptRepository >();
  auto quiz_service = std::make_shared< service::QuizService >(att_repo);
  auto arranger = std::make_shared< tui::RandomAnswerArranger >(42);
  auto runner = std::make_shared< tui::QuizRunner >(terminal, quiz_service, fav_service, arranger, 1);

  tui::FavoritesScreen screen(terminal, fav_service, runner, 1);
  screen.run();

  BOOST_CHECK(terminal->getOutput().find("no favorite questions") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(back_on_action_exits)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"/back"});

  auto fav_repo = std::make_shared< test::MockFavoriteRepository >();
  fav_repo->add(1, 100);
  auto fav_service = std::make_shared< service::FavoriteService >(fav_repo);
  auto att_repo = std::make_shared< test::MockAttemptRepository >();
  auto quiz_service = std::make_shared< service::QuizService >(att_repo);
  auto arranger = std::make_shared< tui::RandomAnswerArranger >(42);
  auto runner = std::make_shared< tui::QuizRunner >(terminal, quiz_service, fav_service, arranger, 1);

  tui::FavoritesScreen screen(terminal, fav_service, runner, 1);
  screen.run();

  BOOST_CHECK(terminal->getOutput().find("Action") != std::string::npos);
  BOOST_CHECK(fav_service->isFavorite(1, 100));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(search_screen_tests)

BOOST_AUTO_TEST_CASE(throws_on_null_dependencies)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  BOOST_CHECK_THROW(tui::SearchScreen(terminal, nullptr, nullptr, 1), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(title_is_search_questions)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  auto client = std::make_shared< MockOpenTdbClient >();
  auto q_repo = std::make_shared< test::MockQuestionRepository >();
  auto question_service = std::make_shared< service::QuestionService >(client, q_repo, 10);
  auto fav_repo = std::make_shared< test::MockFavoriteRepository >();
  auto fav_service = std::make_shared< service::FavoriteService >(fav_repo);

  tui::SearchScreen screen(terminal, question_service, fav_service, 1);
  BOOST_CHECK_EQUAL(screen.title(), "Search questions");
}

BOOST_AUTO_TEST_CASE(back_on_form_exits_without_search)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"/back"});

  auto client = std::make_shared< MockOpenTdbClient >();
  auto q_repo = std::make_shared< test::MockQuestionRepository >();
  auto question_service = std::make_shared< service::QuestionService >(client, q_repo, 10);
  auto fav_repo = std::make_shared< test::MockFavoriteRepository >();
  auto fav_service = std::make_shared< service::FavoriteService >(fav_repo);

  tui::SearchScreen screen(terminal, question_service, fav_service, 1);
  screen.run();

  BOOST_CHECK(terminal->getOutput().find("No questions found.") != std::string::npos ||
              terminal->getOutput().find("Search text") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(custom_test_screen_tests)

BOOST_AUTO_TEST_CASE(throws_on_null_dependencies)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  BOOST_CHECK_THROW(tui::CustomTestScreen(terminal, nullptr, nullptr, nullptr, 1), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(title_is_custom_tests)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  auto ct_repo = std::make_shared< MockCustomTestRepository >();
  auto q_repo = std::make_shared< test::MockQuestionRepository >();
  auto ct_service = std::make_shared< service::CustomTestService >(ct_repo, q_repo);
  auto client = std::make_shared< MockOpenTdbClient >();
  auto question_service = std::make_shared< service::QuestionService >(client, q_repo, 10);
  auto att_repo = std::make_shared< test::MockAttemptRepository >();
  auto quiz_service = std::make_shared< service::QuizService >(att_repo);
  auto fav_repo = std::make_shared< test::MockFavoriteRepository >();
  auto fav_service = std::make_shared< service::FavoriteService >(fav_repo);
  auto arranger = std::make_shared< tui::RandomAnswerArranger >(42);
  auto runner = std::make_shared< tui::QuizRunner >(terminal, quiz_service, fav_service, arranger, 1);

  tui::CustomTestScreen screen(terminal, ct_service, question_service, runner, 1);
  BOOST_CHECK_EQUAL(screen.title(), "Custom tests");
}

BOOST_AUTO_TEST_CASE(back_on_action_exits)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"/back"});

  auto ct_repo = std::make_shared< MockCustomTestRepository >();
  auto q_repo = std::make_shared< test::MockQuestionRepository >();
  auto ct_service = std::make_shared< service::CustomTestService >(ct_repo, q_repo);
  auto client = std::make_shared< MockOpenTdbClient >();
  auto question_service = std::make_shared< service::QuestionService >(client, q_repo, 10);
  auto att_repo = std::make_shared< test::MockAttemptRepository >();
  auto quiz_service = std::make_shared< service::QuizService >(att_repo);
  auto fav_repo = std::make_shared< test::MockFavoriteRepository >();
  auto fav_service = std::make_shared< service::FavoriteService >(fav_repo);
  auto arranger = std::make_shared< tui::RandomAnswerArranger >(42);
  auto runner = std::make_shared< tui::QuizRunner >(terminal, quiz_service, fav_service, arranger, 1);

  tui::CustomTestScreen screen(terminal, ct_service, question_service, runner, 1);
  screen.run();

  BOOST_CHECK(terminal->getOutput().find("Action") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(play_without_tests_shows_message)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"2", "/back"});

  auto ct_repo = std::make_shared< MockCustomTestRepository >();
  auto q_repo = std::make_shared< test::MockQuestionRepository >();
  auto ct_service = std::make_shared< service::CustomTestService >(ct_repo, q_repo);
  auto client = std::make_shared< MockOpenTdbClient >();
  auto question_service = std::make_shared< service::QuestionService >(client, q_repo, 10);
  auto att_repo = std::make_shared< test::MockAttemptRepository >();
  auto quiz_service = std::make_shared< service::QuizService >(att_repo);
  auto fav_repo = std::make_shared< test::MockFavoriteRepository >();
  auto fav_service = std::make_shared< service::FavoriteService >(fav_repo);
  auto arranger = std::make_shared< tui::RandomAnswerArranger >(42);
  auto runner = std::make_shared< tui::QuizRunner >(terminal, quiz_service, fav_service, arranger, 1);

  tui::CustomTestScreen screen(terminal, ct_service, question_service, runner, 1);
  screen.run();

  BOOST_CHECK(terminal->getOutput().find("no custom tests") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(statistics_screen_tests)

BOOST_AUTO_TEST_CASE(throws_on_null_service)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  BOOST_CHECK_THROW(tui::StatisticsScreen(terminal, nullptr, 1), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(title_is_statistics)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  auto stats_repo = std::make_shared< MockStatisticsRepository >();
  auto stats_service = std::make_shared< service::StatisticsService >(stats_repo);

  tui::StatisticsScreen screen(terminal, stats_service, 1);
  BOOST_CHECK_EQUAL(screen.title(), "Statistics");
}

BOOST_AUTO_TEST_CASE(empty_statistics_shows_message)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  auto stats_repo = std::make_shared< MockStatisticsRepository >();
  auto stats_service = std::make_shared< service::StatisticsService >(stats_repo);

  tui::StatisticsScreen screen(terminal, stats_service, 1);
  screen.run();

  BOOST_CHECK(terminal->getOutput().find("No statistics yet") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(nonempty_statistics_shows_overall)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  auto stats_repo = std::make_shared< MockStatisticsRepository >();
  stats_repo->overall.total_attempts = 5;
  stats_repo->overall.total_questions = 20;
  stats_repo->overall.total_correct = 15;
  stats_repo->overall.overall_accuracy = 75.0;
  auto stats_service = std::make_shared< service::StatisticsService >(stats_repo);

  tui::StatisticsScreen screen(terminal, stats_service, 1);
  screen.run();

  const std::string output = terminal->getOutput();
  BOOST_CHECK(output.find("Overall") != std::string::npos);
  BOOST_CHECK(output.find("Attempts") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()
