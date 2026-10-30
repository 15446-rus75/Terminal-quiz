#include "schema_initializer.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "idatabase.hpp"
#include "repository_support.hpp"

namespace
{
  constexpr int SCHEMA_LOCK_NAMESPACE = 742021;
  constexpr int SCHEMA_LOCK_KEY = 1;
}

quiz::db::SchemaInitializer::SchemaInitializer(std::shared_ptr< IDatabase > database): database_(std::move(database))
{
  detail::requireDatabase(database_);
}

void quiz::db::SchemaInitializer::apply()
{
  const std::vector< std::string > STATEMENTS = {
    "CREATE EXTENSION IF NOT EXISTS pg_trgm",
    ("CREATE TABLE IF NOT EXISTS questions ("
     "id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY, "
     "category INTEGER NOT NULL CHECK (category BETWEEN 9 AND 32), "
     "difficulty INTEGER NOT NULL CHECK (difficulty BETWEEN 1 AND 3), "
     "question_type INTEGER NOT NULL CHECK (question_type IN (1,2)), "
     "text TEXT NOT NULL, correct_answer TEXT NOT NULL)"),

    ("CREATE TABLE IF NOT EXISTS question_incorrect_answers ("
     "question_id BIGINT NOT NULL REFERENCES questions(id) ON DELETE CASCADE, "
     "position BIGINT NOT NULL CHECK (position>=0), text TEXT NOT NULL, "
     "PRIMARY KEY(question_id,position))"),

    ("CREATE TABLE IF NOT EXISTS favorites ("
     "user_id BIGINT NOT NULL CHECK (user_id>=0), "
     "question_id BIGINT NOT NULL REFERENCES questions(id) ON DELETE CASCADE, "
     "PRIMARY KEY(user_id,question_id))"),

    ("CREATE TABLE IF NOT EXISTS custom_tests ("
     "id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY, "
     "user_id BIGINT NOT NULL CHECK (user_id>=0), name TEXT NOT NULL, created_at TIMESTAMPTZ NOT NULL)"),

    ("CREATE TABLE IF NOT EXISTS custom_test_questions ("
     "test_id BIGINT NOT NULL REFERENCES custom_tests(id) ON DELETE CASCADE, "
     "question_id BIGINT NOT NULL REFERENCES questions(id), "
     "position BIGINT NOT NULL CHECK (position>=0), PRIMARY KEY(test_id,position))"),

    ("CREATE TABLE IF NOT EXISTS attempts ("
     "id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY, "
     "user_id BIGINT NOT NULL CHECK (user_id>=0), score BIGINT NOT NULL CHECK (score>=0), "
     "total_questions BIGINT NOT NULL CHECK (total_questions>=0 AND score<=total_questions), "
     "started_at TIMESTAMPTZ NOT NULL, finished_at TIMESTAMPTZ, "
     "CHECK (finished_at IS NULL OR finished_at>=started_at))"),

    ("CREATE TABLE IF NOT EXISTS attempt_answers ("
     "id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY, "
     "attempt_id BIGINT NOT NULL REFERENCES attempts(id) ON DELETE CASCADE, "
     "question_id BIGINT NOT NULL REFERENCES questions(id), user_answer TEXT NOT NULL, "
     "is_correct BOOLEAN NOT NULL, position BIGINT NOT NULL CHECK (position>=0), "
     "UNIQUE(attempt_id,position))"),

    "CREATE INDEX IF NOT EXISTS questions_filter_idx ON questions(category,difficulty,id)",
    "CREATE INDEX IF NOT EXISTS questions_text_trgm_idx ON questions USING GIN(text gin_trgm_ops)",
    "CREATE INDEX IF NOT EXISTS favorites_question_idx ON favorites(question_id)",
    "CREATE INDEX IF NOT EXISTS custom_tests_user_idx ON custom_tests(user_id,created_at DESC,id DESC)",
    "CREATE INDEX IF NOT EXISTS custom_test_questions_question_idx ON custom_test_questions(question_id)",
    "CREATE INDEX IF NOT EXISTS attempts_user_idx ON attempts(user_id,started_at DESC,id DESC)",
    "CREATE INDEX IF NOT EXISTS attempt_answers_question_idx ON attempt_answers(question_id)"};

  detail::Transaction transaction(database_);
  transaction.connection().execute("SELECT pg_advisory_xact_lock($1::integer,$2::integer)::text",
                                   {std::to_string(SCHEMA_LOCK_NAMESPACE), std::to_string(SCHEMA_LOCK_KEY)});
  for (const auto &statement : STATEMENTS)
  {
    transaction.connection().execute(statement, {});
  }
  transaction.commit();
}
