#include "repository_support.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../core/attempt.hpp"
#include "../core/enums.hpp"
#include "../core/question.hpp"
#include "idatabase.hpp"

namespace
{
  constexpr long long int MICROSECONDS_PER_SECOND = 1000000;
  constexpr double PERCENT = 100.0;
  constexpr size_t INSERT_BATCH_SIZE = 128;
  constexpr size_t QUESTION_COLUMN_COUNT = 6;
  constexpr size_t QUESTION_ANSWER_COLUMN_COUNT = 2;
  constexpr size_t ATTEMPT_COLUMN_COUNT = 6;
  constexpr size_t ANSWER_COLUMN_COUNT = 6;
  constexpr size_t QUESTION_DIFFICULTY = 2;
  constexpr size_t QUESTION_TYPE = 3;
  constexpr size_t QUESTION_TEXT = 4;
  constexpr size_t QUESTION_CORRECT_ANSWER = 5;
  constexpr size_t ATTEMPT_SCORE = 2;
  constexpr size_t ATTEMPT_TOTAL = 3;
  constexpr size_t ATTEMPT_STARTED = 4;
  constexpr size_t ATTEMPT_FINISHED = 5;
  constexpr size_t ANSWER_QUESTION = 2;
  constexpr size_t ANSWER_TEXT = 3;
  constexpr size_t ANSWER_CORRECT = 4;
  constexpr size_t ANSWER_POSITION = 5;

  template < class Integer >
  Integer parseInteger(const std::string &text)
  {
    Integer value = 0;
    const auto PARSED = std::from_chars(text.data(), text.data() + text.size(), value);
    if (PARSED.ec != std::errc{} || PARSED.ptr != text.data() + text.size())
    {
      throw std::runtime_error("Invalid integer in database result");
    }
    return value;
  }

  bool readBoolean(const std::string &text)
  {
    if (text == "t" || text == "true")
    {
      return true;
    }
    if (text == "f" || text == "false")
    {
      return false;
    }
    throw std::runtime_error("Invalid boolean in database result");
  }

  void insertRows(quiz::db::IConnection &connection, const std::string &prefix,
                  const std::vector< quiz::db::Row > &rows)
  {
    for (size_t start = 0; start < rows.size(); start += INSERT_BATCH_SIZE)
    {
      const size_t END = std::min(start + INSERT_BATCH_SIZE, rows.size());
      std::string sql = prefix + " VALUES ";
      std::vector< std::string > params;
      for (size_t index = start; index < END; ++index)
      {
        quiz::db::detail::requireColumns(rows[index], rows.front().size());
        if (index != start)
        {
          sql += ',';
        }
        sql += '(';
        for (size_t column = 0; column < rows[index].size(); ++column)
        {
          if (column != 0)
          {
            sql += ',';
          }
          params.push_back(rows[index][column]);
          sql += '$' + std::to_string(params.size());
        }
        sql += ')';
      }
      connection.execute(sql, params);
    }
  }

}

void quiz::db::detail::requireDatabase(const std::shared_ptr< IDatabase > &database)
{
  if (!database)
  {
    throw std::invalid_argument("Database must not be null");
  }
}

std::shared_ptr< quiz::db::IConnection > quiz::db::detail::getConnection(const std::shared_ptr< IDatabase > &database)
{
  requireDatabase(database);
  auto connection = database->getConnection();
  if (!connection || !connection->isConnected())
  {
    throw std::runtime_error("Database connection is unavailable");
  }
  return connection;
}

quiz::db::detail::Transaction::Transaction(const std::shared_ptr< IDatabase > &database):
  connection_(getConnection(database)), active_(false)
{
  connection_->beginTransaction();
  active_ = true;
}

quiz::db::detail::Transaction::~Transaction()
{
  if (active_)
  {
    try
    {
      connection_->rollback();
    }
    catch (...)
    {
      connection_.reset();
    }
  }
}

quiz::db::IConnection &quiz::db::detail::Transaction::connection()
{
  return *connection_;
}

void quiz::db::detail::Transaction::commit()
{
  connection_->commit();
  active_ = false;
}

std::string quiz::db::detail::number(size_t value)
{
  if (value > static_cast< size_t >(std::numeric_limits< long long int >::max()))
  {
    throw std::out_of_range("Number exceeds PostgreSQL bigint range");
  }
  return std::to_string(value);
}

size_t quiz::db::detail::readSize(const std::string &text)
{
  const long long int VALUE = parseInteger< long long int >(text);
  if (VALUE < 0 || static_cast< unsigned long long int >(VALUE) > std::numeric_limits< size_t >::max())
  {
    throw std::out_of_range("Database integer is outside the nonnegative size_t range");
  }
  return static_cast< size_t >(VALUE);
}

std::string quiz::db::detail::idArray(const std::vector< size_t > &ids)
{
  std::string result = "{";
  for (size_t index = 0; index < ids.size(); ++index)
  {
    if (index != 0)
    {
      result += ',';
    }
    result += number(ids[index]);
  }
  result += '}';
  return result;
}

void quiz::db::detail::requireColumns(const Row &row, size_t count)
{
  if (row.size() != count)
  {
    throw std::runtime_error("Unexpected database result column count");
  }
}

std::string quiz::db::detail::timeParameter(std::chrono::system_clock::time_point time)
{
  return std::to_string(std::chrono::duration_cast< std::chrono::microseconds >(time.time_since_epoch()).count());
}

std::chrono::system_clock::time_point quiz::db::detail::readTime(const std::string &text)
{
  const long long int VALUE = parseInteger< long long int >(text);
  const auto MINIMUM =
    std::chrono::duration_cast< std::chrono::microseconds >(std::chrono::system_clock::duration::min()).count();
  const auto MAXIMUM =
    std::chrono::duration_cast< std::chrono::microseconds >(std::chrono::system_clock::duration::max()).count();
  if (VALUE < MINIMUM || VALUE > MAXIMUM)
  {
    throw std::out_of_range("Database timestamp exceeds system_clock range");
  }
  return std::chrono::system_clock::time_point(
    std::chrono::duration_cast< std::chrono::system_clock::duration >(std::chrono::microseconds(VALUE)));
}

std::string quiz::db::detail::timestampExpression(const std::string &parameter)
{
  const std::string SCALE = std::to_string(MICROSECONDS_PER_SECOND);
  return "(TIMESTAMPTZ 'epoch' + (" + parameter + "::bigint / " + SCALE + ") * INTERVAL '1 second' + (" + parameter +
         "::bigint % " + SCALE + ") * INTERVAL '1 microsecond')";
}

std::string quiz::db::detail::literalSearchPattern(const std::string &query)
{
  std::string result = "%";
  for (const char CH : query)
  {
    if (CH == '%' || CH == '_' || CH == '\\')
    {
      result.push_back('\\');
    }
    result.push_back(CH);
  }
  result.push_back('%');
  return result;
}

void quiz::db::detail::validateFilter(core::Category category, core::Difficulty difficulty)
{
  core::toString(category);
  core::toString(difficulty);
}

void quiz::db::detail::validateQuestion(const core::Question &question)
{
  validateFilter(question.category, question.difficulty);
  core::toString(question.type);
  if (question.category == core::Category::ANY || question.difficulty == core::Difficulty::ANY ||
      question.type == core::QuestionType::ANY)
  {
    throw std::invalid_argument("Stored question must have a concrete category, difficulty and type");
  }
}

size_t quiz::db::detail::saveQuestion(IConnection &connection, const core::Question &question)
{
  validateQuestion(question);
  std::vector< std::string > params = {
    std::to_string(static_cast< int >(question.category)), std::to_string(static_cast< int >(question.difficulty)),
    std::to_string(static_cast< int >(question.type)), question.text, question.correct_answer};
  std::string sql = "INSERT INTO questions (category, difficulty, question_type, text, correct_answer) "
                    "VALUES ($1, $2, $3, $4, $5) RETURNING id";
  if (question.id != 0)
  {
    params.push_back(number(question.id));
    sql = "UPDATE questions SET category=$1, difficulty=$2, question_type=$3, text=$4, correct_answer=$5 "
          "WHERE id=$6 RETURNING id";
  }
  const ResultSet ROWS = connection.execute(sql, params);
  if (ROWS.size() != 1)
  {
    throw std::out_of_range("Question to update does not exist");
  }
  requireColumns(ROWS.front(), 1);
  const size_t ID = readSize(ROWS.front().front());
  connection.execute("DELETE FROM question_incorrect_answers WHERE question_id=$1", {number(ID)});
  for (size_t position = 0; position < question.incorrect_answers.size(); ++position)
  {
    connection.execute("INSERT INTO question_incorrect_answers (question_id, position, text) VALUES ($1,$2,$3)",
                       {number(ID), number(position), question.incorrect_answers[position]});
  }
  return ID;
}

std::vector< size_t > quiz::db::detail::saveQuestionsBatch(IConnection &connection,
                                                           const std::vector< core::Question > &questions)
{
  std::vector< size_t > ids;
  ids.reserve(questions.size());
  size_t start = 0;
  while (start < questions.size())
  {
    if (questions[start].id != 0)
    {
      ids.push_back(saveQuestion(connection, questions[start]));
      ++start;
      continue;
    }
    size_t end = start;
    while (end < questions.size() && end - start < INSERT_BATCH_SIZE && questions[end].id == 0)
    {
      validateQuestion(questions[end]);
      ++end;
    }
    const ResultSet ALLOCATED = connection.execute(
      "SELECT nextval(pg_get_serial_sequence('questions','id')) FROM generate_series(1,$1::integer) AS ordinal "
      "ORDER BY ordinal",
      {number(end - start)});
    if (ALLOCATED.size() != end - start)
    {
      throw std::runtime_error("Unexpected number of allocated question IDs");
    }
    std::vector< Row > records;
    std::vector< Row > answers;
    records.reserve(end - start);
    for (size_t index = start; index < end; ++index)
    {
      requireColumns(ALLOCATED[index - start], 1);
      const size_t ID = readSize(ALLOCATED[index - start].front());
      const auto &question = questions[index];
      records.push_back({number(ID), std::to_string(static_cast< int >(question.category)),
                         std::to_string(static_cast< int >(question.difficulty)),
                         std::to_string(static_cast< int >(question.type)), question.text, question.correct_answer});
      for (size_t position = 0; position < question.incorrect_answers.size(); ++position)
      {
        answers.push_back({number(ID), number(position), question.incorrect_answers[position]});
      }
      ids.push_back(ID);
    }
    insertRows(connection,
               "INSERT INTO questions(id,category,difficulty,question_type,text,correct_answer) "
               "OVERRIDING SYSTEM VALUE",
               records);
    insertRows(connection, "INSERT INTO question_incorrect_answers(question_id,position,text)", answers);
    start = end;
  }
  return ids;
}

std::vector< quiz::core::Question > quiz::db::detail::readQuestions(IConnection &connection, const ResultSet &rows)
{
  std::vector< core::Question > result;
  result.reserve(rows.size());
  std::vector< size_t > ids;
  ids.reserve(rows.size());
  std::unordered_map< size_t, size_t > positions;
  for (const auto &row : rows)
  {
    requireColumns(row, QUESTION_COLUMN_COUNT);
    const size_t CATEGORY = readSize(row[1]);
    const size_t DIFFICULTY = readSize(row[QUESTION_DIFFICULTY]);
    const size_t TYPE = readSize(row[QUESTION_TYPE]);
    constexpr size_t INT_MAXIMUM = static_cast< size_t >(std::numeric_limits< int >::max());
    if (CATEGORY > INT_MAXIMUM || DIFFICULTY > INT_MAXIMUM || TYPE > INT_MAXIMUM)
    {
      throw std::runtime_error("Database enum exceeds int range");
    }
    core::Question question{};
    question.id = readSize(row[0]);
    question.category = static_cast< core::Category >(CATEGORY);
    question.difficulty = static_cast< core::Difficulty >(DIFFICULTY);
    question.type = static_cast< core::QuestionType >(TYPE);
    question.text = row[QUESTION_TEXT];
    question.correct_answer = row[QUESTION_CORRECT_ANSWER];
    validateQuestion(question);
    if (!positions.emplace(question.id, result.size()).second)
    {
      throw std::runtime_error("Duplicate question in database result");
    }
    ids.push_back(question.id);
    result.push_back(std::move(question));
  }
  if (!ids.empty())
  {
    const ResultSet ANSWERS =
      connection.execute("SELECT question_id,text FROM question_incorrect_answers WHERE question_id=ANY($1::bigint[]) "
                         "ORDER BY question_id,position",
                         {idArray(ids)});
    for (const auto &answer : ANSWERS)
    {
      requireColumns(answer, QUESTION_ANSWER_COLUMN_COUNT);
      result.at(positions.at(readSize(answer[0]))).incorrect_answers.push_back(answer[1]);
    }
  }
  return result;
}

void quiz::db::detail::replaceAnswers(IConnection &connection, size_t attempt_id,
                                      const std::vector< core::AttemptAnswer > &answers)
{
  const ResultSet ATTEMPT = connection.execute("SELECT id FROM attempts WHERE id=$1 FOR UPDATE", {number(attempt_id)});
  if (ATTEMPT.empty())
  {
    throw std::out_of_range("Attempt does not exist");
  }
  connection.execute("DELETE FROM attempt_answers WHERE attempt_id=$1", {number(attempt_id)});
  for (const auto &answer : answers)
  {
    if (answer.attempt_id != 0 && answer.attempt_id != attempt_id)
    {
      throw std::invalid_argument("Answer belongs to a different attempt");
    }
    connection.execute("INSERT INTO attempt_answers (attempt_id,question_id,user_answer,is_correct,position) "
                       "VALUES ($1,$2,$3,$4,$5)",
                       {number(attempt_id), number(answer.question_id), answer.user_answer,
                        answer.is_correct ? "true" : "false", number(answer.position)});
  }
}

std::vector< quiz::core::Attempt > quiz::db::detail::readAttempts(IConnection &connection, const ResultSet &rows)
{
  std::vector< core::Attempt > result;
  result.reserve(rows.size());
  std::vector< size_t > ids;
  ids.reserve(rows.size());
  std::unordered_map< size_t, size_t > positions;
  for (const auto &row : rows)
  {
    requireColumns(row, ATTEMPT_COLUMN_COUNT);
    core::Attempt attempt{};
    attempt.id = readSize(row[0]);
    attempt.user_id = readSize(row[1]);
    attempt.score = readSize(row[ATTEMPT_SCORE]);
    attempt.total_questions = readSize(row[ATTEMPT_TOTAL]);
    attempt.started_at = readTime(row[ATTEMPT_STARTED]);
    if (!row[ATTEMPT_FINISHED].empty())
    {
      attempt.finished_at = readTime(row[ATTEMPT_FINISHED]);
    }
    if (!positions.emplace(attempt.id, result.size()).second)
    {
      throw std::runtime_error("Duplicate attempt in database result");
    }
    ids.push_back(attempt.id);
    result.push_back(std::move(attempt));
  }
  if (!ids.empty())
  {
    const ResultSet ANSWERS =
      connection.execute("SELECT id,attempt_id,question_id,user_answer,is_correct,position FROM attempt_answers "
                         "WHERE attempt_id=ANY($1::bigint[]) ORDER BY attempt_id,position,id",
                         {idArray(ids)});
    for (const auto &answer : ANSWERS)
    {
      requireColumns(answer, ANSWER_COLUMN_COUNT);
      const size_t ATTEMPT_ID = readSize(answer[1]);
      result.at(positions.at(ATTEMPT_ID))
        .answers.push_back({readSize(answer[0]), ATTEMPT_ID, readSize(answer[ANSWER_QUESTION]), answer[ANSWER_TEXT],
                            readBoolean(answer[ANSWER_CORRECT]), readSize(answer[ANSWER_POSITION])});
    }
  }
  return result;
}

double quiz::db::detail::accuracy(size_t correct, size_t total)
{
  return total == 0 ? 0.0 : PERCENT * static_cast< double >(correct) / static_cast< double >(total);
}
