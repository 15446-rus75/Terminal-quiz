#include "postgres_question_repository.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../core/enums.hpp"
#include "../core/question.hpp"
#include "idatabase.hpp"
#include "repository_support.hpp"

quiz::db::PostgresQuestionRepository::PostgresQuestionRepository(std::shared_ptr< IDatabase > database):
  database_(std::move(database))
{
  detail::requireDatabase(database_);
}

std::optional< quiz::core::Question > quiz::db::PostgresQuestionRepository::findById(size_t id)
{
  const auto CONNECTION = detail::getConnection(database_);
  const ResultSet ROWS = CONNECTION->execute(
    std::string("SELECT ") + detail::QUESTION_COLUMNS + " FROM questions WHERE id=$1", {detail::number(id)});
  auto entries = detail::readQuestions(*CONNECTION, ROWS);
  if (entries.empty())
  {
    return std::nullopt;
  }
  std::optional< core::Question > result(std::in_place, std::move(entries.front()));
  return result;
}

std::vector< quiz::core::Question >
quiz::db::PostgresQuestionRepository::findByFilter(core::Category category, core::Difficulty difficulty, size_t limit)
{
  detail::validateFilter(category, difficulty);
  const auto CONNECTION = detail::getConnection(database_);
  const ResultSet ROWS = CONNECTION->execute(
    std::string("SELECT ") + detail::QUESTION_COLUMNS +
      " FROM questions WHERE ($1::integer=0 OR category=$1) AND ($2::integer=0 OR difficulty=$2) ORDER BY id LIMIT $3",
    {std::to_string(static_cast< int >(category)), std::to_string(static_cast< int >(difficulty)),
     detail::number(limit)});
  auto result = detail::readQuestions(*CONNECTION, ROWS);
  return result;
}

std::vector< quiz::core::Question > quiz::db::PostgresQuestionRepository::searchByText(const std::string &query,
                                                                                       core::Category category,
                                                                                       core::Difficulty difficulty,
                                                                                       size_t limit)
{
  detail::validateFilter(category, difficulty);
  const auto CONNECTION = detail::getConnection(database_);
  const ResultSet ROWS =
    CONNECTION->execute(std::string("SELECT ") + detail::QUESTION_COLUMNS +
                          " FROM questions WHERE text ILIKE $1 ESCAPE E'\\\\' AND ($2::integer=0 OR category=$2) "
                          "AND ($3::integer=0 OR difficulty=$3) ORDER BY id LIMIT $4",
                        {detail::literalSearchPattern(query), std::to_string(static_cast< int >(category)),
                         std::to_string(static_cast< int >(difficulty)), detail::number(limit)});
  auto result = detail::readQuestions(*CONNECTION, ROWS);
  return result;
}

size_t quiz::db::PostgresQuestionRepository::save(const core::Question &question)
{
  detail::Transaction transaction(database_);
  const size_t ID = detail::saveQuestion(transaction.connection(), question);
  transaction.commit();
  return ID;
}

std::vector< size_t > quiz::db::PostgresQuestionRepository::saveBatch(const std::vector< core::Question > &questions)
{
  detail::Transaction transaction(database_);
  auto ids = detail::saveQuestionsBatch(transaction.connection(), questions);
  transaction.commit();
  return ids;
}

size_t quiz::db::PostgresQuestionRepository::count()
{
  const auto CONNECTION = detail::getConnection(database_);
  const ResultSet ROWS = CONNECTION->execute("SELECT COUNT(*) FROM questions", {});
  if (ROWS.size() != 1)
  {
    throw std::runtime_error("Unexpected question count result");
  }
  detail::requireColumns(ROWS.front(), 1);
  const size_t COUNT = detail::readSize(ROWS.front().front());
  return COUNT;
}
