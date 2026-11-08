#include <chrono>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "app/application.hpp"
#include "app/config.hpp"
#include "core/enums.hpp"
#include "core/question.hpp"
#include "db/postgres_attempt_repository.hpp"
#include "db/postgres_database.hpp"
#include "db/postgres_favorite_repository.hpp"
#include "db/postgres_question_repository.hpp"
#include "db/schema_initializer.hpp"
#include "tui/terminal.hpp"
#include <boost/test/unit_test.hpp>

namespace
{
  quiz::core::Question makeSeedQuestion(size_t n)
  {
    quiz::core::Question q;
    q.id = 0;
    q.category = quiz::core::Category::GENERAL_KNOWLEDGE;
    q.difficulty = quiz::core::Difficulty::EASY;
    q.type = quiz::core::QuestionType::MULTIPLE;
    q.text = "Seeded integration question " + std::to_string(n);
    q.correct_answer = "yes";
    q.incorrect_answers = {"no", "maybe", "never"};
    return q;
  }

  struct AppRunResult
  {
    int code = -1;
    std::string output;
  };

  struct AppIntegrationFixture
  {
    AppIntegrationFixture()
    {
      config = quiz::app::defaultConfig();
      const char *DB_NAME = std::getenv("QUIZ_TEST_DB");
      config.db_name = (DB_NAME != nullptr && *DB_NAME != '\0') ? DB_NAME : "quiz_test";
      config.use_colors = false;
      config.cache_capacity = 10;

      database = std::make_shared< quiz::db::PostgresDatabase >(config.db_host, config.db_port, config.db_name,
                                                                config.db_user, config.db_password);
      quiz::db::SchemaInitializer(database).apply();

      auto connection = database->getConnection();
      connection->execute("TRUNCATE attempts, attempt_answers, favorites, custom_tests, "
                          "custom_test_questions, questions, question_incorrect_answers "
                          "RESTART IDENTITY CASCADE",
                          {});

      question_repository = std::make_shared< quiz::db::PostgresQuestionRepository >(database);
      attempt_repository = std::make_shared< quiz::db::PostgresAttemptRepository >(database);
      favorite_repository = std::make_shared< quiz::db::PostgresFavoriteRepository >(database);
      question_repository->saveBatch({makeSeedQuestion(1), makeSeedQuestion(2), makeSeedQuestion(3)});
    }

    AppRunResult runApp(const std::string &script) const
    {
      std::istringstream input(script);
      std::ostringstream output;
      auto terminal = std::make_shared< quiz::tui::StreamTerminal >(input, output, false);
      quiz::app::Application application(config, terminal);
      AppRunResult result;
      result.code = application.run();
      result.output = output.str();
      return result;
    }

    quiz::app::AppConfig config;
    std::shared_ptr< quiz::db::PostgresDatabase > database;
    std::shared_ptr< quiz::db::PostgresQuestionRepository > question_repository;
    std::shared_ptr< quiz::db::PostgresAttemptRepository > attempt_repository;
    std::shared_ptr< quiz::db::PostgresFavoriteRepository > favorite_repository;
  };

  const std::string QUIZ_PARAMS = "3\n1\n1\n1\n";
}

BOOST_FIXTURE_TEST_SUITE(app_integration_tests, AppIntegrationFixture)

BOOST_AUTO_TEST_CASE(quiz_playthrough_persists_finished_attempt)
{
  const AppRunResult RESULT = runApp("1\n" + QUIZ_PARAMS + "1\n1\n1\n" + "0\n");

  BOOST_CHECK_EQUAL(RESULT.code, 0);
  BOOST_CHECK(RESULT.output.find("Question 1 of 3") != std::string::npos);
  BOOST_CHECK(RESULT.output.find("Quiz finished") != std::string::npos);
  BOOST_CHECK(RESULT.output.find("Score:") != std::string::npos);

  const auto ATTEMPTS = attempt_repository->findByUserId(config.user_id, 10);
  BOOST_REQUIRE_EQUAL(ATTEMPTS.size(), 1u);
  BOOST_CHECK(ATTEMPTS[0].finished_at != std::chrono::system_clock::time_point{});
  BOOST_CHECK_EQUAL(ATTEMPTS[0].total_questions, 3u);
  BOOST_CHECK_EQUAL(ATTEMPTS[0].answers.size(), 3u);
}

BOOST_AUTO_TEST_CASE(history_and_statistics_reflect_completed_attempt)
{
  const AppRunResult RESULT = runApp("1\n" + QUIZ_PARAMS + "1\n1\n1\n" + "5\n1\n/back\n6\n0\n");

  BOOST_CHECK_EQUAL(RESULT.code, 0);
  BOOST_CHECK(RESULT.output.find("History") != std::string::npos);
  BOOST_CHECK(RESULT.output.find("Attempt #") != std::string::npos);
  BOOST_CHECK(RESULT.output.find("Your answer:") != std::string::npos);
  BOOST_CHECK(RESULT.output.find("Overall") != std::string::npos);
  BOOST_CHECK(RESULT.output.find("By category") != std::string::npos);

  const auto ATTEMPTS = attempt_repository->findByUserId(config.user_id, 10);
  BOOST_REQUIRE_EQUAL(ATTEMPTS.size(), 1u);
}

BOOST_AUTO_TEST_CASE(fav_command_adds_question_and_favorites_screen_lists_it)
{
  const AppRunResult RESULT = runApp("1\n" + QUIZ_PARAMS + "/fav\n1\n1\n1\n" + "3\n/back\n0\n");

  BOOST_CHECK_EQUAL(RESULT.code, 0);
  BOOST_CHECK(RESULT.output.find("Added to favorites") != std::string::npos);
  BOOST_CHECK(RESULT.output.find("Action (/back to return)") != std::string::npos);

  const auto FAVORITES = favorite_repository->findByUserId(config.user_id);
  BOOST_CHECK_EQUAL(FAVORITES.size(), 1u);
}

BOOST_AUTO_TEST_CASE(search_finds_seeded_questions_and_adds_them_to_favorites)
{
  const AppRunResult RESULT = runApp("4\nSeeded\n1\n1\n5\ny\n1-2\n0\n");

  BOOST_CHECK_EQUAL(RESULT.code, 0);
  BOOST_CHECK(RESULT.output.find("Seeded integration question") != std::string::npos);
  BOOST_CHECK(RESULT.output.find("Added 2 question(s) to favorites") != std::string::npos);

  const auto FAVORITES = favorite_repository->findByUserId(config.user_id);
  BOOST_CHECK_EQUAL(FAVORITES.size(), 2u);
}

BOOST_AUTO_TEST_CASE(stop_command_finishes_attempt_with_partial_answers)
{
  const AppRunResult RESULT = runApp("1\n" + QUIZ_PARAMS + "1\n/stop\n0\n");

  BOOST_CHECK_EQUAL(RESULT.code, 0);
  BOOST_CHECK(RESULT.output.find("Quiz finished") != std::string::npos);

  const auto ATTEMPTS = attempt_repository->findByUserId(config.user_id, 10);
  BOOST_REQUIRE_EQUAL(ATTEMPTS.size(), 1u);
  BOOST_CHECK_EQUAL(ATTEMPTS[0].total_questions, 3u);
  BOOST_CHECK_EQUAL(ATTEMPTS[0].answers.size(), 1u);
  BOOST_CHECK(ATTEMPTS[0].finished_at != std::chrono::system_clock::time_point{});
  BOOST_CHECK(ATTEMPTS[0].score <= 1u);
}

BOOST_AUTO_TEST_SUITE_END()
