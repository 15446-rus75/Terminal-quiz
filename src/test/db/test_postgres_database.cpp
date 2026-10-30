#include <chrono>
#include <cstdlib>
#include <future>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "test_support.hpp"
#include <boost/test/unit_test.hpp>

using namespace quiz;
using namespace dbtest;
BOOST_AUTO_TEST_SUITE(postgres)

BOOST_AUTO_TEST_CASE(parameter_round_trip_is_not_sql_interpolation)
{
  auto source = database();
  auto connection = source->getConnection();
  const std::string TEXT = "O'Reilly \\ UTF-8 Ж; DROP TABLE questions; --";
  BOOST_CHECK(connection->isConnected());
  BOOST_CHECK(connection->execute("SELECT $1::text, $2::text", {TEXT, ""}) == (db::ResultSet{{TEXT, ""}}));
  BOOST_CHECK(connection->execute("SELECT 1 WHERE FALSE", {}).empty());
}

BOOST_AUTO_TEST_CASE(null_bytes_and_unrepresentable_sql_null_are_errors)
{
  auto source = database();
  auto connection = source->getConnection();
  BOOST_CHECK_THROW(connection->execute("SELECT $1::text", {std::string("x\0y", 3)}), std::invalid_argument);
  BOOST_CHECK_THROW(connection->execute(std::string("SELECT 1\0x", 10), {}), std::invalid_argument);
  BOOST_CHECK_THROW(connection->execute("SELECT NULL::text", {}), std::runtime_error);
  BOOST_CHECK(connection->execute("SELECT 2", {}) == (db::ResultSet{{"2"}}));
}

BOOST_AUTO_TEST_CASE(query_failure_does_not_poison_autocommit_connection)
{
  auto source = database();
  auto connection = source->getConnection();
  BOOST_CHECK_THROW(connection->execute("SELECT 1 / 0", {}), std::runtime_error);
  BOOST_CHECK_THROW(connection->execute("SELECT missing syntax", {"bad"}), std::runtime_error);
  BOOST_CHECK(connection->execute("SELECT $1::integer", {"8"}) == (db::ResultSet{{"8"}}));
}

BOOST_AUTO_TEST_CASE(explicit_commit_and_rollback_control_visibility)
{
  auto source = database();
  auto writer = source->getConnection();
  auto reader = source->getConnection();
  writer->execute("DROP TABLE IF EXISTS transaction_visibility", {});
  writer->execute("CREATE TABLE transaction_visibility(value integer)", {});
  writer->beginTransaction();
  writer->execute("INSERT INTO transaction_visibility VALUES(1)", {});
  BOOST_CHECK_EQUAL(reader->execute("SELECT COUNT(*) FROM transaction_visibility", {}).at(0).at(0), "0");
  writer->rollback();
  writer->rollback();
  BOOST_CHECK_EQUAL(reader->execute("SELECT COUNT(*) FROM transaction_visibility", {}).at(0).at(0), "0");
  writer->beginTransaction();
  writer->execute("INSERT INTO transaction_visibility VALUES(2)", {});
  writer->commit();
  BOOST_CHECK_EQUAL(reader->execute("SELECT value FROM transaction_visibility", {}).at(0).at(0), "2");
  writer->execute("DROP TABLE transaction_visibility", {});
}

BOOST_AUTO_TEST_CASE(transaction_lifecycle_errors_are_explicit)
{
  auto source = database();
  auto connection = source->getConnection();
  BOOST_CHECK_THROW(connection->commit(), std::logic_error);
  connection->beginTransaction();
  BOOST_CHECK_THROW(connection->beginTransaction(), std::logic_error);
  BOOST_CHECK_THROW(connection->execute("SELECT 1 / 0", {}), std::runtime_error);
  BOOST_CHECK_THROW(connection->commit(), std::logic_error);
  connection->rollback();
  BOOST_CHECK_EQUAL(connection->execute("SELECT 3", {}).at(0).at(0), "3");
}

BOOST_AUTO_TEST_CASE(connection_parameters_cannot_inject_database_option)
{
  const char *socket = std::getenv("QUIZ_TEST_SOCKET");
  const char *user = std::getenv("QUIZ_TEST_USER");
  const char *isolated = std::getenv("QUIZ_TEST_ISOLATED");
  BOOST_REQUIRE(socket != nullptr);
  BOOST_REQUIRE(user != nullptr);
  BOOST_REQUIRE(isolated != nullptr);
  BOOST_REQUIRE(*socket != '\0');
  BOOST_REQUIRE(*user != '\0');
  BOOST_REQUIRE_EQUAL(std::string(isolated), "1");
  db::PostgresDatabase quoted(socket, 5432, "postgres", user, "' dbname=missing_database \\");
  BOOST_CHECK(quoted.getConnection()->isConnected());
  db::PostgresDatabase missing(socket, 5432, "missing_database", user, "");
  BOOST_CHECK_THROW(missing.getConnection(), std::runtime_error);
  for (size_t index = 0; index < 5; ++index)
  {
    BOOST_CHECK_THROW(missing.getConnection(), std::runtime_error);
  }
}

BOOST_AUTO_TEST_CASE(returned_connection_reuses_backend_after_last_alias)
{
  auto source = database();
  auto first = source->getConnection();
  auto alias = first;
  const auto PID = first->execute("SELECT pg_backend_pid()", {}).at(0).at(0);
  first.reset();
  auto second = source->getConnection();
  BOOST_CHECK_NE(second->execute("SELECT pg_backend_pid()", {}).at(0).at(0), PID);
  alias.reset();
  auto third = source->getConnection();
  BOOST_CHECK_EQUAL(third->execute("SELECT pg_backend_pid()", {}).at(0).at(0), PID);
}

BOOST_AUTO_TEST_CASE(pool_has_four_distinct_leases_and_times_out_when_exhausted, *boost::unit_test::timeout(20))
{
  auto source = database();
  std::vector< std::shared_ptr< db::IConnection > > held;
  std::set< std::string > pids;
  for (size_t index = 0; index < 4; ++index)
  {
    held.push_back(source->getConnection());
    pids.insert(held.back()->execute("SELECT pg_backend_pid()", {}).at(0).at(0));
  }
  BOOST_CHECK_EQUAL(pids.size(), 4);
  BOOST_CHECK_THROW(source->getConnection(), std::runtime_error);
  held.pop_back();
  BOOST_CHECK(source->getConnection()->isConnected());
}

BOOST_AUTO_TEST_CASE(waiting_borrower_resumes_after_release, *boost::unit_test::timeout(20))
{
  auto source = database();
  std::vector< std::shared_ptr< db::IConnection > > held;
  for (size_t index = 0; index < 4; ++index)
  {
    held.push_back(source->getConnection());
  }
  std::promise< void > started;
  auto waiter = std::async(std::launch::async,
                           [&]()
                           {
                             started.set_value();
                             return source->getConnection();
                           });
  started.get_future().wait();
  const auto BEFORE = waiter.wait_for(std::chrono::milliseconds(50));
  held.pop_back();
  BOOST_CHECK(BEFORE == std::future_status::timeout);
  BOOST_CHECK(waiter.get()->isConnected());
}

BOOST_AUTO_TEST_CASE(concurrent_operations_use_separate_connections, *boost::unit_test::timeout(20))
{
  auto source = database();
  std::vector< std::future< std::string > > workers;
  for (size_t index = 0; index < 12; ++index)
  {
    workers.push_back(std::async(std::launch::async,
                                 [source, index]()
                                 {
                                   auto connection = source->getConnection();
                                   return connection->execute("SELECT $1::text", {std::to_string(index)}).at(0).at(0);
                                 }));
  }
  for (size_t index = 0; index < workers.size(); ++index)
  {
    BOOST_CHECK_EQUAL(workers[index].get(), std::to_string(index));
  }
}

BOOST_AUTO_TEST_CASE(pool_rolls_back_abandoned_and_failed_transactions)
{
  auto source = database();
  auto connection = source->getConnection();
  connection->execute("CREATE TEMP TABLE lease_probe(value integer)", {});
  const auto PID = connection->execute("SELECT pg_backend_pid()", {}).at(0).at(0);
  connection->beginTransaction();
  connection->execute("INSERT INTO lease_probe VALUES(1)", {});
  connection.reset();
  connection = source->getConnection();
  BOOST_CHECK_EQUAL(connection->execute("SELECT pg_backend_pid()", {}).at(0).at(0), PID);
  BOOST_CHECK_EQUAL(connection->execute("SELECT COUNT(*) FROM lease_probe", {}).at(0).at(0), "0");
  connection->beginTransaction();
  BOOST_CHECK_THROW(connection->execute("SELECT 1 / 0", {}), std::runtime_error);
  connection.reset();
  connection = source->getConnection();
  BOOST_CHECK_EQUAL(connection->execute("SELECT 7", {}).at(0).at(0), "7");
}

BOOST_AUTO_TEST_CASE(broken_backend_is_replaced)
{
  auto source = database();
  auto victim = source->getConnection();
  auto admin = source->getConnection();
  const auto PID = victim->execute("SELECT pg_backend_pid()", {}).at(0).at(0);
  admin->execute("SELECT pg_terminate_backend($1::integer)", {PID});
  BOOST_CHECK_THROW(victim->execute("SELECT 1", {}), std::runtime_error);
  victim.reset();
  auto replacement = source->getConnection();
  BOOST_CHECK_NE(replacement->execute("SELECT pg_backend_pid()", {}).at(0).at(0), PID);
}

BOOST_AUTO_TEST_CASE(lease_can_outlive_database_owner)
{
  auto source = database();
  auto lease = source->getConnection();
  source.reset();
  BOOST_CHECK_EQUAL(lease->execute("SELECT 9", {}).at(0).at(0), "9");
  lease.reset();
}

BOOST_AUTO_TEST_CASE(prepared_statement_is_reused_with_new_parameters_and_after_pool_return)
{
  auto source = database();
  auto connection = source->getConnection();
  for (const auto &value : {"first", "second", "third"})
  {
    BOOST_CHECK_EQUAL(connection->execute("SELECT $1::text AS reused", {value}).at(0).at(0), value);
  }
  BOOST_CHECK_EQUAL(connection->execute("SELECT COUNT(*) FROM pg_prepared_statements", {}).at(0).at(0), "1");
  connection.reset();
  connection = source->getConnection();
  BOOST_CHECK_EQUAL(connection->execute("SELECT $1::text AS reused", {"fourth"}).at(0).at(0), "fourth");
  BOOST_CHECK_EQUAL(connection->execute("SELECT COUNT(*) FROM pg_prepared_statements", {}).at(0).at(0), "1");
}

BOOST_AUTO_TEST_CASE(prepared_cache_is_bounded_and_fallback_still_binds_parameters)
{
  auto source = database();
  auto connection = source->getConnection();
  for (size_t index = 0; index < 70; ++index)
  {
    const auto TEXT = std::to_string(index);
    BOOST_CHECK_EQUAL(connection->execute("SELECT $1::text /* variant " + TEXT + " */", {TEXT}).at(0).at(0), TEXT);
  }
  BOOST_CHECK_EQUAL(connection->execute("SELECT COUNT(*) FROM pg_prepared_statements", {}).at(0).at(0), "64");
  const auto PAYLOAD = "' ; DROP TABLE questions;--";
  BOOST_CHECK_EQUAL(connection->execute("SELECT $1::text AS fallback", {PAYLOAD}).at(0).at(0), PAYLOAD);
}

BOOST_AUTO_TEST_SUITE_END()
