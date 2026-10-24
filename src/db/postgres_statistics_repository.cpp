#include "postgres_statistics_repository.hpp"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../core/enums.hpp"
#include "../core/statistics.hpp"
#include "idatabase.hpp"
#include "repository_support.hpp"

namespace
{
  constexpr size_t OVERALL_COLUMN_COUNT = 3;
  constexpr size_t GROUP_COLUMN_COUNT = 4;
  constexpr size_t CORRECT_COLUMN = 2;
  constexpr size_t GROUP_TOTAL_COLUMN = 3;

  std::string groupedStatisticsSql(const std::string &column)
  {
    return "SELECT q." + column +
           ",COUNT(DISTINCT a.id),COUNT(*) FILTER (WHERE ans.is_correct),COUNT(*) "
           "FROM attempts a JOIN attempt_answers ans ON ans.attempt_id=a.id JOIN questions q ON q.id=ans.question_id "
           "WHERE a.user_id=$1 AND a.finished_at IS NOT NULL GROUP BY q." +
           column + " ORDER BY q." + column;
  }
}

quiz::db::PostgresStatisticsRepository::PostgresStatisticsRepository(std::shared_ptr< IDatabase > database):
  database_(std::move(database))
{
  detail::requireDatabase(database_);
}

quiz::core::OverallStats quiz::db::PostgresStatisticsRepository::getOverall(size_t user_id)
{
  detail::Transaction transaction(database_);
  const ResultSet ROWS =
    transaction.connection().execute("SELECT COUNT(*),COALESCE(SUM(total_questions),0),COALESCE(SUM(score),0) "
                                     "FROM attempts WHERE user_id=$1 AND finished_at IS NOT NULL",
                                     {detail::number(user_id)});
  if (ROWS.size() != 1)
  {
    throw std::runtime_error("Unexpected overall statistics result");
  }
  detail::requireColumns(ROWS.front(), OVERALL_COLUMN_COUNT);
  core::OverallStats result{};
  result.total_attempts = detail::readSize(ROWS.front()[0]);
  result.total_questions = detail::readSize(ROWS.front()[1]);
  result.total_correct = detail::readSize(ROWS.front()[CORRECT_COLUMN]);
  result.overall_accuracy = detail::accuracy(result.total_correct, result.total_questions);
  transaction.commit();
  return result;
}

std::vector< quiz::core::CategoryStats > quiz::db::PostgresStatisticsRepository::getByCategory(size_t user_id)
{
  detail::Transaction transaction(database_);
  const ResultSet ROWS = transaction.connection().execute(groupedStatisticsSql("category"), {detail::number(user_id)});
  std::vector< core::CategoryStats > result;
  result.reserve(ROWS.size());
  for (const auto &row : ROWS)
  {
    detail::requireColumns(row, GROUP_COLUMN_COUNT);
    const auto CATEGORY = static_cast< core::Category >(detail::readSize(row[0]));
    detail::validateFilter(CATEGORY, core::Difficulty::ANY);
    const size_t ATTEMPTS = detail::readSize(row[1]);
    const size_t CORRECT = detail::readSize(row[CORRECT_COLUMN]);
    const size_t TOTAL = detail::readSize(row[GROUP_TOTAL_COLUMN]);
    result.push_back({CATEGORY, ATTEMPTS, CORRECT, TOTAL, detail::accuracy(CORRECT, TOTAL)});
  }
  transaction.commit();
  return result;
}

std::vector< quiz::core::DifficultyStats > quiz::db::PostgresStatisticsRepository::getByDifficulty(size_t user_id)
{
  detail::Transaction transaction(database_);
  const ResultSet ROWS =
    transaction.connection().execute(groupedStatisticsSql("difficulty"), {detail::number(user_id)});
  std::vector< core::DifficultyStats > result;
  result.reserve(ROWS.size());
  for (const auto &row : ROWS)
  {
    detail::requireColumns(row, GROUP_COLUMN_COUNT);
    const auto DIFFICULTY = static_cast< core::Difficulty >(detail::readSize(row[0]));
    detail::validateFilter(core::Category::ANY, DIFFICULTY);
    const size_t ATTEMPTS = detail::readSize(row[1]);
    const size_t CORRECT = detail::readSize(row[CORRECT_COLUMN]);
    const size_t TOTAL = detail::readSize(row[GROUP_TOTAL_COLUMN]);
    result.push_back({DIFFICULTY, ATTEMPTS, CORRECT, TOTAL, detail::accuracy(CORRECT, TOTAL)});
  }
  transaction.commit();
  return result;
}
