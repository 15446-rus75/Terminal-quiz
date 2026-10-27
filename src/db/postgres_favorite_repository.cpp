#include "postgres_favorite_repository.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../core/question.hpp"
#include "idatabase.hpp"
#include "repository_support.hpp"

quiz::db::PostgresFavoriteRepository::PostgresFavoriteRepository(std::shared_ptr< IDatabase > database):
  database_(std::move(database))
{
  detail::requireDatabase(database_);
}

void quiz::db::PostgresFavoriteRepository::add(size_t user_id, size_t question_id)
{
  detail::Transaction transaction(database_);
  transaction.connection().execute("INSERT INTO favorites (user_id,question_id) VALUES ($1,$2) "
                                   "ON CONFLICT (user_id,question_id) DO NOTHING",
                                   {detail::number(user_id), detail::number(question_id)});
  transaction.commit();
}

void quiz::db::PostgresFavoriteRepository::remove(size_t user_id, size_t question_id)
{
  detail::Transaction transaction(database_);
  transaction.connection().execute("DELETE FROM favorites WHERE user_id=$1 AND question_id=$2",
                                   {detail::number(user_id), detail::number(question_id)});
  transaction.commit();
}

std::vector< quiz::core::Question > quiz::db::PostgresFavoriteRepository::findByUserId(size_t user_id)
{
  const auto CONNECTION = detail::getConnection(database_);
  const ResultSet ROWS = CONNECTION->execute(
    std::string("SELECT ") + detail::QUESTION_COLUMNS +
      " FROM questions WHERE id IN (SELECT question_id FROM favorites WHERE user_id=$1) ORDER BY id",
    {detail::number(user_id)});
  auto result = detail::readQuestions(*CONNECTION, ROWS);
  return result;
}

bool quiz::db::PostgresFavoriteRepository::exists(size_t user_id, size_t question_id)
{
  const auto CONNECTION = detail::getConnection(database_);
  const ResultSet ROWS = CONNECTION->execute("SELECT 1 FROM favorites WHERE user_id=$1 AND question_id=$2",
                                             {detail::number(user_id), detail::number(question_id)});
  const bool EXISTS = !ROWS.empty();
  return EXISTS;
}
