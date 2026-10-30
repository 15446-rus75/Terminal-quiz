#include "test_support.hpp"

#include <cstdlib>
#include <mutex>
#include <stdexcept>
#include <utility>

#include "db/schema_initializer.hpp"
#include <boost/test/unit_test.hpp>

namespace
{
  std::string requiredEnvironment(const char *name)
  {
    const char *value = std::getenv(name);
    if (value == nullptr || *value == '\0')
    {
      throw std::runtime_error(std::string("Run integration tests through make test; missing ") + name);
    }
    return value;
  }

  class CountedConnection: public quiz::db::IConnection
  {
  public:
    CountedConnection(std::shared_ptr< quiz::db::IConnection > source, std::shared_ptr< dbtest::Calls > calls):
      source_(std::move(source)), calls_(std::move(calls))
    {
    }
    bool isConnected() const override
    {
      return source_->isConnected();
    }
    void beginTransaction() override
    {
      ++calls_->begins;
      source_->beginTransaction();
    }
    void commit() override
    {
      ++calls_->commits;
      source_->commit();
    }
    void rollback() override
    {
      source_->rollback();
    }
    quiz::db::ResultSet execute(const std::string &sql, const std::vector< std::string > &params) override
    {
      ++calls_->queries;
      return source_->execute(sql, params);
    }

  private:
    std::shared_ptr< quiz::db::IConnection > source_;
    std::shared_ptr< dbtest::Calls > calls_;
  };
}

std::shared_ptr< quiz::db::PostgresDatabase > dbtest::database()
{
  if (requiredEnvironment("QUIZ_TEST_ISOLATED") != "1")
  {
    throw std::runtime_error("An isolated test cluster is required");
  }
  return std::make_shared< quiz::db::PostgresDatabase >(requiredEnvironment("QUIZ_TEST_SOCKET"), 5432, "postgres",
                                                        requiredEnvironment("QUIZ_TEST_USER"), "");
}

quiz::core::Question dbtest::question(const std::string &text)
{
  return {0,
          quiz::core::Category::SCIENCE_COMPUTERS,
          quiz::core::Difficulty::EASY,
          quiz::core::QuestionType::MULTIPLE,
          text,
          "correct",
          {"third", "first", "second"}};
}

quiz::core::Attempt dbtest::attempt(size_t user, size_t score, size_t total, bool finished)
{
  const auto START = std::chrono::system_clock::time_point(std::chrono::microseconds(1700000000123456));
  return {0,     user,  score,
          total, START, finished ? START + std::chrono::seconds(10) : std::chrono::system_clock::time_point{},
          {}};
}

quiz::core::CustomTest dbtest::customTest(const std::vector< size_t > &ids)
{
  return {0, USER, "Тест O'Reilly", std::chrono::system_clock::time_point(std::chrono::microseconds(-1234567)), ids};
}

void dbtest::checkQuestion(const quiz::core::Question &actual, const quiz::core::Question &expected)
{
  BOOST_CHECK_EQUAL(actual.text, expected.text);
  BOOST_CHECK_EQUAL(actual.correct_answer, expected.correct_answer);
  BOOST_CHECK(actual.category == expected.category);
  BOOST_CHECK(actual.difficulty == expected.difficulty);
  BOOST_CHECK(actual.type == expected.type);
  BOOST_CHECK_EQUAL_COLLECTIONS(actual.incorrect_answers.begin(), actual.incorrect_answers.end(),
                                expected.incorrect_answers.begin(), expected.incorrect_answers.end());
}

dbtest::Fixture::Fixture(): db(database()), questions(db), favorites(db), tests(db), attempts(db), statistics(db)
{
  static std::once_flag initialized;
  std::call_once(initialized,
                 [this]()
                 {
                   quiz::db::SchemaInitializer(db).apply();
                 });
  db->getConnection()->execute("TRUNCATE attempt_answers, attempts, custom_test_questions, custom_tests, "
                               "favorites, question_incorrect_answers, questions RESTART IDENTITY",
                               {});
}

dbtest::CountedDatabase::CountedDatabase(std::shared_ptr< quiz::db::IDatabase > source):
  calls(std::make_shared< Calls >()), source_(std::move(source))
{
}

void dbtest::Fixture::deleteQuestion(size_t id)
{
  db->getConnection()->execute("DELETE FROM questions WHERE id=$1", {std::to_string(id)});
}

std::shared_ptr< quiz::db::IConnection > dbtest::CountedDatabase::getConnection()
{
  return std::make_shared< CountedConnection >(source_->getConnection(), calls);
}
