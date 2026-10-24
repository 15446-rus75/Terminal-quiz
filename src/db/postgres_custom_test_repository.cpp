#include "postgres_custom_test_repository.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../core/custom_test.hpp"
#include "idatabase.hpp"
#include "repository_support.hpp"

namespace
{
  constexpr size_t TEST_COLUMN_COUNT = 4;
  constexpr size_t NAME_COLUMN = 2;
  constexpr size_t CREATED_COLUMN = 3;
  constexpr char TEST_COLUMNS[] = "id,user_id,name,(EXTRACT(EPOCH FROM created_at)*1000000)::bigint::text";

  std::vector< quiz::core::CustomTest > readTests(quiz::db::IConnection &connection, const quiz::db::ResultSet &rows)
  {
    std::vector< quiz::core::CustomTest > result;
    result.reserve(rows.size());
    for (const auto &row : rows)
    {
      quiz::db::detail::requireColumns(row, TEST_COLUMN_COUNT);
      quiz::core::CustomTest test{};
      test.id = quiz::db::detail::readSize(row[0]);
      test.user_id = quiz::db::detail::readSize(row[1]);
      test.name = row[NAME_COLUMN];
      test.created_at = quiz::db::detail::readTime(row[CREATED_COLUMN]);
      const quiz::db::ResultSet QUESTIONS =
        connection.execute("SELECT question_id FROM custom_test_questions WHERE test_id=$1 ORDER BY position",
                           {quiz::db::detail::number(test.id)});
      for (const auto &question : QUESTIONS)
      {
        quiz::db::detail::requireColumns(question, 1);
        test.question_ids.push_back(quiz::db::detail::readSize(question.front()));
      }
      result.push_back(std::move(test));
    }
    return result;
  }
}

quiz::db::PostgresCustomTestRepository::PostgresCustomTestRepository(std::shared_ptr< IDatabase > database):
  database_(std::move(database))
{
  detail::requireDatabase(database_);
}

size_t quiz::db::PostgresCustomTestRepository::save(const core::CustomTest &test)
{
  detail::Transaction transaction(database_);
  std::vector< std::string > params = {detail::number(test.user_id), test.name, detail::timeParameter(test.created_at)};
  const std::string CREATED_AT = detail::timestampExpression("$3");
  std::string sql = "INSERT INTO custom_tests (user_id,name,created_at) VALUES ($1,$2," + CREATED_AT + ") RETURNING id";
  if (test.id != 0)
  {
    params.push_back(detail::number(test.id));
    sql = "UPDATE custom_tests SET user_id=$1,name=$2,created_at=" + CREATED_AT + " WHERE id=$4 RETURNING id";
  }
  const ResultSet ROWS = transaction.connection().execute(sql, params);
  if (ROWS.size() != 1)
  {
    throw std::out_of_range("Custom test to update does not exist");
  }
  detail::requireColumns(ROWS.front(), 1);
  const size_t ID = detail::readSize(ROWS.front().front());
  saveQuestions(transaction.connection(), ID, test.question_ids);
  transaction.commit();
  return ID;
}

void quiz::db::PostgresCustomTestRepository::saveQuestions(IConnection &connection, size_t test_id,
                                                           const std::vector< size_t > &question_ids)
{
  connection.execute("DELETE FROM custom_test_questions WHERE test_id=$1", {detail::number(test_id)});
  for (size_t position = 0; position < question_ids.size(); ++position)
  {
    connection.execute("INSERT INTO custom_test_questions (test_id,question_id,position) VALUES ($1,$2,$3)",
                       {detail::number(test_id), detail::number(question_ids[position]), detail::number(position)});
  }
}

std::optional< quiz::core::CustomTest > quiz::db::PostgresCustomTestRepository::findById(size_t id)
{
  detail::Transaction transaction(database_);
  const ResultSet ROWS = transaction.connection().execute(
    std::string("SELECT ") + TEST_COLUMNS + " FROM custom_tests WHERE id=$1", {detail::number(id)});
  auto result = readTests(transaction.connection(), ROWS);
  transaction.commit();
  if (result.empty())
  {
    return std::nullopt;
  }
  return std::move(result.front());
}

std::vector< quiz::core::CustomTest > quiz::db::PostgresCustomTestRepository::findByUserId(size_t user_id)
{
  detail::Transaction transaction(database_);
  const ResultSet ROWS = transaction.connection().execute(
    std::string("SELECT ") + TEST_COLUMNS + " FROM custom_tests WHERE user_id=$1 ORDER BY created_at DESC,id DESC",
    {detail::number(user_id)});
  auto result = readTests(transaction.connection(), ROWS);
  transaction.commit();
  return result;
}

void quiz::db::PostgresCustomTestRepository::remove(size_t id)
{
  detail::Transaction transaction(database_);
  transaction.connection().execute("DELETE FROM custom_tests WHERE id=$1", {detail::number(id)});
  transaction.commit();
}
