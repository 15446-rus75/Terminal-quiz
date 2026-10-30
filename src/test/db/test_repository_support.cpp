#include <chrono>
#include <deque>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "db/repository_support.hpp"
#include "db/schema_initializer.hpp"
#include "test_support.hpp"
#include <boost/test/unit_test.hpp>

namespace
{
  class FakeConnection: public quiz::db::IConnection
  {
  public:
    bool connected = true;
    bool fail_begin = false;
    bool fail_commit = false;
    bool fail_rollback = false;
    size_t begins = 0;
    size_t commits = 0;
    size_t rollbacks = 0;
    size_t queries = 0;
    std::deque< quiz::db::ResultSet > results;
    bool isConnected() const override
    {
      return connected;
    }
    void beginTransaction() override
    {
      ++begins;
      if (fail_begin)
      {
        throw std::runtime_error("begin failed");
      }
    }
    void commit() override
    {
      ++commits;
      if (fail_commit)
      {
        throw std::runtime_error("commit failed");
      }
    }
    void rollback() override
    {
      ++rollbacks;
      if (fail_rollback)
      {
        throw std::runtime_error("rollback failed");
      }
    }
    quiz::db::ResultSet execute(const std::string &, const std::vector< std::string > &) override
    {
      ++queries;
      if (results.empty())
      {
        throw std::runtime_error("Unexpected query in fake");
      }
      auto result = results.front();
      results.pop_front();
      return result;
    }
  };
  class FakeDatabase: public quiz::db::IDatabase
  {
  public:
    std::shared_ptr< FakeConnection > connection = std::make_shared< FakeConnection >();
    std::shared_ptr< quiz::db::IConnection > getConnection() override
    {
      return connection;
    }
  };
}

using namespace quiz;
BOOST_AUTO_TEST_SUITE(support)

BOOST_AUTO_TEST_CASE(null_dependencies_are_rejected)
{
  BOOST_CHECK_THROW(db::PostgresQuestionRepository(nullptr), std::invalid_argument);
  BOOST_CHECK_THROW(db::PostgresFavoriteRepository(nullptr), std::invalid_argument);
  BOOST_CHECK_THROW(db::PostgresCustomTestRepository(nullptr), std::invalid_argument);
  BOOST_CHECK_THROW(db::PostgresAttemptRepository(nullptr), std::invalid_argument);
  BOOST_CHECK_THROW(db::PostgresStatisticsRepository(nullptr), std::invalid_argument);
  BOOST_CHECK_THROW(db::SchemaInitializer(nullptr), std::invalid_argument);
  BOOST_CHECK_THROW(db::detail::getConnection(nullptr), std::invalid_argument);
}
BOOST_AUTO_TEST_CASE(unavailable_connections_are_rejected)
{
  auto database = std::make_shared< FakeDatabase >();
  database->connection->connected = false;
  BOOST_CHECK_THROW(db::detail::getConnection(database), std::runtime_error);
  database->connection.reset();
  BOOST_CHECK_THROW(db::detail::getConnection(database), std::runtime_error);
}
BOOST_AUTO_TEST_CASE(port_and_connection_parameters_are_validated_without_network)
{
  BOOST_CHECK_THROW(db::PostgresDatabase("localhost", 0, "x", "x", ""), std::invalid_argument);
  BOOST_CHECK_THROW(db::PostgresDatabase("localhost", 65536, "x", "x", ""), std::invalid_argument);
  const std::string BAD("x\0y", 3);
  BOOST_CHECK_THROW(db::PostgresDatabase(BAD, 5432, "x", "x", ""), std::invalid_argument);
  BOOST_CHECK_THROW(db::PostgresDatabase("localhost", 5432, BAD, "x", ""), std::invalid_argument);
  BOOST_CHECK_THROW(db::PostgresDatabase("localhost", 5432, "x", BAD, ""), std::invalid_argument);
  BOOST_CHECK_THROW(db::PostgresDatabase("localhost", 5432, "x", "x", BAD), std::invalid_argument);
}
BOOST_AUTO_TEST_CASE(integer_results_reject_negative_overflow_and_malformed_text)
{
  BOOST_CHECK_EQUAL(db::detail::readSize("0"), 0);
  BOOST_CHECK_EQUAL(db::detail::readSize("42"), 42);
  for (const auto &text : {"-1", "-9223372036854775808", "9223372036854775808", "", " 1", "1 ", "+1", "1x"})
  {
    BOOST_TEST_CONTEXT("value=" << text)
    {
      BOOST_CHECK_THROW(db::detail::readSize(text), std::exception);
    }
  }
}
BOOST_AUTO_TEST_CASE(bigint_parameters_reject_out_of_range_values)
{
  BOOST_CHECK_EQUAL(db::detail::number(0), "0");
  BOOST_CHECK_EQUAL(db::detail::number(42), "42");
  if (std::numeric_limits< size_t >::max() > static_cast< unsigned long long >(std::numeric_limits< long long >::max()))
  {
    BOOST_CHECK_THROW(db::detail::number(std::numeric_limits< size_t >::max()), std::out_of_range);
  }
}
BOOST_AUTO_TEST_CASE(microsecond_times_round_trip_before_and_after_epoch)
{
  for (const long long VALUE : {0LL, 1234567LL, -1234567LL, 1700000000123456LL})
  {
    const auto TIME = std::chrono::system_clock::time_point(std::chrono::microseconds(VALUE));
    BOOST_CHECK(db::detail::readTime(db::detail::timeParameter(TIME)) == TIME);
  }
  BOOST_CHECK_THROW(db::detail::readTime("not a time"), std::runtime_error);
  BOOST_CHECK_THROW(db::detail::readTime("9223372036854775808"), std::runtime_error);
}
BOOST_AUTO_TEST_CASE(search_patterns_escape_only_sql_wildcards)
{
  BOOST_CHECK_EQUAL(db::detail::literalSearchPattern("100%_a\\b'"), "%100\\%\\_a\\\\b'%");
  BOOST_CHECK_EQUAL(db::detail::literalSearchPattern(""), "%%");
  BOOST_CHECK_EQUAL(db::detail::idArray({}), "{}");
  BOOST_CHECK_EQUAL(db::detail::idArray({2, 9}), "{2,9}");
}
BOOST_AUTO_TEST_CASE(accuracy_has_percent_units_and_handles_zero_denominator)
{
  BOOST_CHECK_SMALL(db::detail::accuracy(0, 0), 1e-10);
  BOOST_CHECK_CLOSE(db::detail::accuracy(1, 4), 25.0, 1e-8);
  BOOST_CHECK_CLOSE(db::detail::accuracy(2, 3), 200.0 / 3.0, 1e-8);
}
BOOST_AUTO_TEST_CASE(stored_questions_require_concrete_enums)
{
  auto value = dbtest::question();
  BOOST_CHECK_NO_THROW(db::detail::validateQuestion(value));
  value.category = core::Category::ANY;
  BOOST_CHECK_THROW(db::detail::validateQuestion(value), std::invalid_argument);
  value = dbtest::question();
  value.difficulty = core::Difficulty::ANY;
  BOOST_CHECK_THROW(db::detail::validateQuestion(value), std::invalid_argument);
  value = dbtest::question();
  value.type = core::QuestionType::ANY;
  BOOST_CHECK_THROW(db::detail::validateQuestion(value), std::invalid_argument);
  BOOST_CHECK_NO_THROW(db::detail::validateFilter(core::Category::ANY, core::Difficulty::ANY));
  BOOST_CHECK_THROW(db::detail::validateFilter(static_cast< core::Category >(999), core::Difficulty::ANY),
                    std::exception);
}
BOOST_AUTO_TEST_CASE(transaction_commits_once_without_rollback)
{
  auto database = std::make_shared< FakeDatabase >();
  {
    db::detail::Transaction transaction(database);
    transaction.commit();
  }
  BOOST_CHECK_EQUAL(database->connection->begins, 1);
  BOOST_CHECK_EQUAL(database->connection->commits, 1);
  BOOST_CHECK_EQUAL(database->connection->rollbacks, 0);
}
BOOST_AUTO_TEST_CASE(uncommitted_transaction_rolls_back_on_scope_exit)
{
  auto database = std::make_shared< FakeDatabase >();
  {
    db::detail::Transaction transaction(database);
  }
  BOOST_CHECK_EQUAL(database->connection->rollbacks, 1);
}
BOOST_AUTO_TEST_CASE(failed_commit_is_rolled_back)
{
  auto database = std::make_shared< FakeDatabase >();
  database->connection->fail_commit = true;
  {
    db::detail::Transaction transaction(database);
    BOOST_CHECK_THROW(transaction.commit(), std::runtime_error);
  }
  BOOST_CHECK_EQUAL(database->connection->rollbacks, 1);
}
BOOST_AUTO_TEST_CASE(failed_begin_does_not_rollback_unstarted_transaction)
{
  auto database = std::make_shared< FakeDatabase >();
  database->connection->fail_begin = true;
  BOOST_CHECK_THROW(db::detail::Transaction transaction(database), std::runtime_error);
  BOOST_CHECK_EQUAL(database->connection->rollbacks, 0);
}
BOOST_AUTO_TEST_CASE(rollback_failure_does_not_escape_destructor)
{
  auto database = std::make_shared< FakeDatabase >();
  database->connection->fail_rollback = true;
  BOOST_CHECK_NO_THROW({ db::detail::Transaction transaction(database); });
  BOOST_CHECK_EQUAL(database->connection->rollbacks, 1);
}
BOOST_AUTO_TEST_CASE(empty_parent_results_do_not_query_children)
{
  FakeConnection connection;
  BOOST_CHECK(db::detail::readQuestions(connection, {}).empty());
  BOOST_CHECK(db::detail::readAttempts(connection, {}).empty());
  BOOST_CHECK_EQUAL(connection.queries, 0);
}
BOOST_AUTO_TEST_CASE(question_mapping_preserves_parent_order_and_batches_children)
{
  FakeConnection connection;
  connection.results.push_back({{"1", "a"}, {"1", "b"}, {"2", "c"}});
  const auto VALUES =
    db::detail::readQuestions(connection, {{"2", "18", "1", "1", "two", "yes"}, {"1", "18", "1", "1", "one", "yes"}});
  BOOST_REQUIRE_EQUAL(VALUES.size(), 2);
  BOOST_CHECK_EQUAL(VALUES[0].id, 2);
  BOOST_REQUIRE_EQUAL(VALUES[0].incorrect_answers.size(), 1);
  BOOST_CHECK_EQUAL(VALUES[0].incorrect_answers[0], "c");
  BOOST_REQUIRE_EQUAL(VALUES[1].incorrect_answers.size(), 2);
  BOOST_CHECK_EQUAL(VALUES[1].incorrect_answers[1], "b");
  BOOST_CHECK_EQUAL(connection.queries, 1);
}
BOOST_AUTO_TEST_CASE(malformed_question_rows_and_duplicates_are_rejected)
{
  FakeConnection connection;
  BOOST_CHECK_THROW(db::detail::readQuestions(connection, {{"1"}}), std::runtime_error);
  BOOST_CHECK_THROW(db::detail::readQuestions(connection, {{"1", "2147483648", "1", "1", "q", "a"}}),
                    std::runtime_error);
  const db::Row ROW{"1", "18", "1", "1", "q", "a"};
  BOOST_CHECK_THROW(db::detail::readQuestions(connection, {ROW, ROW}), std::runtime_error);
  connection.results.push_back({{"999", "orphan"}});
  BOOST_CHECK_THROW(db::detail::readQuestions(connection, {ROW}), std::out_of_range);
}
BOOST_AUTO_TEST_CASE(attempt_mapping_checks_boolean_and_column_count)
{
  FakeConnection connection;
  const db::ResultSet PARENTS{{"7", "42", "1", "2", "1234567", ""}};
  connection.results.push_back({{"1", "7", "9", "text", "true", "0"}, {"2", "7", "9", "other", "f", "1"}});
  const auto VALUES = db::detail::readAttempts(connection, PARENTS);
  BOOST_REQUIRE_EQUAL(VALUES.size(), 1);
  BOOST_REQUIRE_EQUAL(VALUES[0].answers.size(), 2);
  BOOST_CHECK(VALUES[0].answers[0].is_correct);
  BOOST_CHECK(!VALUES[0].answers[1].is_correct);
  BOOST_CHECK(VALUES[0].finished_at == std::chrono::system_clock::time_point{});
  connection.results.push_back({{"1", "7", "9", "text", "maybe", "0"}});
  BOOST_CHECK_THROW(db::detail::readAttempts(connection, PARENTS), std::runtime_error);
  BOOST_CHECK_THROW(db::detail::readAttempts(connection, {{"1"}}), std::runtime_error);
}
BOOST_AUTO_TEST_CASE(statistics_reject_invalid_aggregate_results)
{
  auto database = std::make_shared< FakeDatabase >();
  db::PostgresStatisticsRepository repository(database);
  database->connection->results.push_back({{"-1", "0", "0"}});
  BOOST_CHECK_THROW(repository.getOverall(42), std::out_of_range);
  database->connection->results.push_back({});
  BOOST_CHECK_THROW(repository.getOverall(42), std::runtime_error);
  database->connection->results.push_back({{"1", "2"}});
  BOOST_CHECK_THROW(repository.getOverall(42), std::runtime_error);
  database->connection->results.push_back({{"999", "1", "1", "1"}});
  BOOST_CHECK_THROW(repository.getByCategory(42), std::exception);
}
BOOST_AUTO_TEST_SUITE_END()
