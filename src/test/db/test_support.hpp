#ifndef QUIZ_DB_TEST_SUPPORT_HPP
#define QUIZ_DB_TEST_SUPPORT_HPP

#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "db/postgres_attempt_repository.hpp"
#include "db/postgres_custom_test_repository.hpp"
#include "db/postgres_database.hpp"
#include "db/postgres_favorite_repository.hpp"
#include "db/postgres_question_repository.hpp"
#include "db/postgres_statistics_repository.hpp"

namespace dbtest
{
  inline constexpr size_t USER = 42;
  inline constexpr size_t OTHER_USER = 73;
  inline constexpr size_t MISSING = 999999;

  std::shared_ptr< quiz::db::PostgresDatabase > database();
  quiz::core::Question question(const std::string &text = "Question");
  quiz::core::Attempt attempt(size_t user = USER, size_t score = 1, size_t total = 2, bool finished = true);
  quiz::core::CustomTest customTest(const std::vector< size_t > &ids = {});
  void checkQuestion(const quiz::core::Question &actual, const quiz::core::Question &expected);

  struct Fixture
  {
    Fixture();
    void deleteQuestion(size_t id);
    std::shared_ptr< quiz::db::PostgresDatabase > db;
    quiz::db::PostgresQuestionRepository questions;
    quiz::db::PostgresFavoriteRepository favorites;
    quiz::db::PostgresCustomTestRepository tests;
    quiz::db::PostgresAttemptRepository attempts;
    quiz::db::PostgresStatisticsRepository statistics;
  };

  struct Calls
  {
    size_t queries = 0;
    size_t begins = 0;
    size_t commits = 0;
  };

  class CountedDatabase: public quiz::db::IDatabase
  {
  public:
    explicit CountedDatabase(std::shared_ptr< quiz::db::IDatabase > source);
    std::shared_ptr< quiz::db::IConnection > getConnection() override;
    std::shared_ptr< Calls > calls;

  private:
    std::shared_ptr< quiz::db::IDatabase > source_;
  };
}
#endif
