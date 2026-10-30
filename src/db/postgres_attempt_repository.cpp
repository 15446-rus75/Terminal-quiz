#include "postgres_attempt_repository.hpp"

#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../core/attempt.hpp"
#include "idatabase.hpp"
#include "repository_support.hpp"

quiz::db::PostgresAttemptRepository::PostgresAttemptRepository(std::shared_ptr< IDatabase > database):
  database_(std::move(database))
{
  detail::requireDatabase(database_);
}

size_t quiz::db::PostgresAttemptRepository::save(const core::Attempt &attempt)
{
  detail::Transaction transaction(database_);
  const bool FINISHED = attempt.finished_at != std::chrono::system_clock::time_point{};
  std::vector< std::string > params = {
    detail::number(attempt.user_id), detail::number(attempt.score), detail::number(attempt.total_questions),
    detail::timeParameter(attempt.started_at), FINISHED ? detail::timeParameter(attempt.finished_at) : ""};
  const std::string STARTED_AT = detail::timestampExpression("$4");
  const std::string FINISHED_AT = "CASE WHEN $5='' THEN NULL ELSE " + detail::timestampExpression("$5") + " END";
  std::string sql = "INSERT INTO attempts (user_id,score,total_questions,started_at,finished_at) VALUES ($1,$2,$3," +
                    STARTED_AT + "," + FINISHED_AT + ") RETURNING id";
  if (attempt.id != 0)
  {
    params.push_back(detail::number(attempt.id));
    sql = "UPDATE attempts SET user_id=$1,score=$2,total_questions=$3,started_at=" + STARTED_AT +
          ",finished_at=" + FINISHED_AT + " WHERE id=$6 RETURNING id";
  }
  const ResultSet ROWS = transaction.connection().execute(sql, params);
  if (ROWS.size() != 1)
  {
    throw std::out_of_range("Attempt to update does not exist");
  }
  detail::requireColumns(ROWS.front(), 1);
  const size_t ID = detail::readSize(ROWS.front().front());
  detail::replaceAnswers(transaction.connection(), ID, attempt.answers);
  transaction.commit();
  return ID;
}

void quiz::db::PostgresAttemptRepository::saveAnswers(size_t attempt_id,
                                                      const std::vector< core::AttemptAnswer > &answers)
{
  detail::Transaction transaction(database_);
  detail::replaceAnswers(transaction.connection(), attempt_id, answers);
  transaction.commit();
}

void quiz::db::PostgresAttemptRepository::finish(size_t attempt_id, size_t score,
                                                 std::chrono::system_clock::time_point finished_at)
{
  if (finished_at == std::chrono::system_clock::time_point{})
  {
    throw std::invalid_argument("Finish time must not be the unfinished-attempt sentinel");
  }
  detail::Transaction transaction(database_);
  const ResultSet ROWS = transaction.connection().execute(
    "UPDATE attempts SET score=$2,finished_at=" + detail::timestampExpression("$3") + " WHERE id=$1 RETURNING id",
    {detail::number(attempt_id), detail::number(score), detail::timeParameter(finished_at)});
  if (ROWS.empty())
  {
    throw std::out_of_range("Attempt does not exist");
  }
  transaction.commit();
}

std::vector< quiz::core::Attempt > quiz::db::PostgresAttemptRepository::findByUserId(size_t user_id, size_t limit)
{
  const auto CONNECTION = detail::getConnection(database_);
  const ResultSet ROWS =
    CONNECTION->execute(std::string("SELECT ") + detail::ATTEMPT_COLUMNS +
                          " FROM attempts WHERE user_id=$1 ORDER BY started_at DESC,id DESC LIMIT $2",
                        {detail::number(user_id), detail::number(limit)});
  auto result = detail::readAttempts(*CONNECTION, ROWS);
  return result;
}

std::optional< quiz::core::Attempt > quiz::db::PostgresAttemptRepository::findById(size_t id)
{
  const auto CONNECTION = detail::getConnection(database_);
  const ResultSet ROWS = CONNECTION->execute(
    std::string("SELECT ") + detail::ATTEMPT_COLUMNS + " FROM attempts WHERE id=$1", {detail::number(id)});
  auto entries = detail::readAttempts(*CONNECTION, ROWS);
  if (entries.empty())
  {
    return std::nullopt;
  }
  std::optional< core::Attempt > result(std::in_place, std::move(entries.front()));
  return result;
}
