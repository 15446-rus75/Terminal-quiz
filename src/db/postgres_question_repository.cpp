#include "postgres_question_repository.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
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

namespace
{
  class QuestionSaver
  {
  public:
    explicit QuestionSaver(quiz::db::IConnection &connection);
    size_t operator()(const quiz::core::Question &question) const;

  private:
    quiz::db::IConnection &connection_;
  };

  QuestionSaver::QuestionSaver(quiz::db::IConnection &connection): connection_(connection)
  {
  }

  size_t QuestionSaver::operator()(const quiz::core::Question &question) const
  {
    return quiz::db::detail::saveQuestion(connection_, question);
  }
}

quiz::db::PostgresQuestionRepository::PostgresQuestionRepository(std::shared_ptr< IDatabase > database):
  database_(std::move(database))
{
  detail::requireDatabase(database_);
}

std::optional< quiz::core::Question > quiz::db::PostgresQuestionRepository::findById(size_t id)
{
  detail::Transaction transaction(database_);
  const ResultSet ROWS = transaction.connection().execute(
    std::string("SELECT ") + detail::QUESTION_COLUMNS + " FROM questions WHERE id=$1", {detail::number(id)});
  auto result = detail::readQuestions(transaction.connection(), ROWS);
  transaction.commit();
  if (result.empty())
  {
    return std::nullopt;
  }
  return std::move(result.front());
}

std::vector< quiz::core::Question >
quiz::db::PostgresQuestionRepository::findByFilter(core::Category category, core::Difficulty difficulty, size_t limit)
{
  detail::validateFilter(category, difficulty);
  detail::Transaction transaction(database_);
  const ResultSet ROWS = transaction.connection().execute(
    std::string("SELECT ") + detail::QUESTION_COLUMNS +
      " FROM questions WHERE ($1::integer=0 OR category=$1) AND ($2::integer=0 OR difficulty=$2) ORDER BY id LIMIT $3",
    {std::to_string(static_cast< int >(category)), std::to_string(static_cast< int >(difficulty)),
     detail::number(limit)});
  auto result = detail::readQuestions(transaction.connection(), ROWS);
  transaction.commit();
  return result;
}

std::vector< quiz::core::Question > quiz::db::PostgresQuestionRepository::searchByText(const std::string &query,
                                                                                       core::Category category,
                                                                                       core::Difficulty difficulty,
                                                                                       size_t limit)
{
  detail::validateFilter(category, difficulty);
  detail::Transaction transaction(database_);
  const ResultSet ROWS = transaction.connection().execute(
    std::string("SELECT ") + detail::QUESTION_COLUMNS +
      " FROM questions WHERE text ILIKE $1 ESCAPE E'\\\\' AND ($2::integer=0 OR category=$2) "
      "AND ($3::integer=0 OR difficulty=$3) ORDER BY id LIMIT $4",
    {detail::literalSearchPattern(query), std::to_string(static_cast< int >(category)),
     std::to_string(static_cast< int >(difficulty)), detail::number(limit)});
  auto result = detail::readQuestions(transaction.connection(), ROWS);
  transaction.commit();
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
  std::vector< size_t > ids;
  ids.reserve(questions.size());
  std::transform(questions.begin(), questions.end(), std::back_inserter(ids), QuestionSaver(transaction.connection()));
  transaction.commit();
  return ids;
}

size_t quiz::db::PostgresQuestionRepository::count()
{
  detail::Transaction transaction(database_);
  const ResultSet ROWS = transaction.connection().execute("SELECT COUNT(*) FROM questions", {});
  if (ROWS.size() != 1)
  {
    throw std::runtime_error("Unexpected question count result");
  }
  detail::requireColumns(ROWS.front(), 1);
  const size_t COUNT = detail::readSize(ROWS.front().front());
  transaction.commit();
  return COUNT;
}
