#include "question_service.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../api/open_tdb_client.hpp"
#include "../core/enums.hpp"
#include "../core/question.hpp"
#include "../db/iquestion_repository.hpp"
#include "../util/logger.hpp"

namespace
{
  constexpr size_t HASH_MAGIC = 0x9e3779b9;
  constexpr size_t HASH_LEFT_SHIFT = 6;
  constexpr size_t HASH_RIGHT_SHIFT = 2;
  constexpr size_t TYPED_FETCH_FACTOR = 4;

  size_t combineHash(size_t seed, size_t value)
  {
    return seed ^ (value + HASH_MAGIC + (seed << HASH_LEFT_SHIFT) + (seed >> HASH_RIGHT_SHIFT));
  }

  class HasOtherType
  {
  public:
    explicit HasOtherType(quiz::core::QuestionType type): type_(type)
    {
    }

    bool operator()(const quiz::core::Question &question) const
    {
      return question.type != type_;
    }

  private:
    quiz::core::QuestionType type_;
  };

  class IdAssigner
  {
  public:
    quiz::core::Question operator()(quiz::core::Question question, size_t id) const
    {
      question.id = id;
      return question;
    }
  };
}

size_t quiz::service::QuestionService::CacheKeyHash::operator()(const CacheKey &key) const
{
  size_t seed = std::hash< size_t >{}(key.amount);
  seed = combineHash(seed, std::hash< int >{}(static_cast< int >(key.category)));
  seed = combineHash(seed, std::hash< int >{}(static_cast< int >(key.difficulty)));
  seed = combineHash(seed, std::hash< int >{}(static_cast< int >(key.type)));
  return seed;
}

quiz::service::QuestionService::QuestionService(std::shared_ptr< api::IOpenTdbClient > open_tdb_client,
                                                std::shared_ptr< db::IQuestionRepository > question_repository,
                                                size_t cache_capacity):
  open_tdb_client_(std::move(open_tdb_client)),
  question_repository_(std::move(question_repository)),
  cache_(cache_capacity)
{
  if (open_tdb_client_ == nullptr)
  {
    throw std::invalid_argument("Open Trivia DB client must not be null");
  }
  if (question_repository_ == nullptr)
  {
    throw std::invalid_argument("Question repository must not be null");
  }
}

std::vector< quiz::core::Question > quiz::service::QuestionService::fetchQuestions(size_t amount,
                                                                                   core::Category category,
                                                                                   core::Difficulty difficulty,
                                                                                   core::QuestionType type)
{
  const CacheKey KEY{.amount = amount, .category = category, .difficulty = difficulty, .type = type};
  std::optional< std::vector< core::Question > > cached = cache_.get(KEY);
  if (cached.has_value())
  {
    return std::move(*cached);
  }

  std::vector< core::Question > questions;
  try
  {
    questions = fetchFromApi(KEY);
  }
  catch (const std::runtime_error &error)
  {
    util::Logger::warning(std::string("Open Trivia DB is unavailable, using local database: ") + error.what());
    questions = fetchFromDb(KEY);
    if (questions.empty())
    {
      throw;
    }
    return questions;
  }

  if (questions.empty())
  {
    return fetchFromDb(KEY);
  }
  persistToDb(questions);
  cache_.put(KEY, questions);
  return questions;
}

std::vector< quiz::core::Question > quiz::service::QuestionService::searchByText(const std::string &query,
                                                                                 core::Category category,
                                                                                 core::Difficulty difficulty,
                                                                                 size_t limit)
{
  if (limit == 0)
  {
    return {};
  }
  return question_repository_->searchByText(query, category, difficulty, limit);
}

std::optional< quiz::core::Question > quiz::service::QuestionService::findById(size_t id)
{
  return question_repository_->findById(id);
}

std::vector< quiz::core::Question > quiz::service::QuestionService::fetchFromApi(const CacheKey &key)
{
  const api::OpenTdbRequest REQUEST{
    .amount = key.amount, .category = key.category, .difficulty = key.difficulty, .type = key.type};
  return open_tdb_client_->fetchQuestions(REQUEST);
}

std::vector< quiz::core::Question > quiz::service::QuestionService::fetchFromDb(const CacheKey &key)
{
  const bool ANY_TYPE = key.type == core::QuestionType::ANY;
  const size_t LIMIT = ANY_TYPE ? key.amount : key.amount * TYPED_FETCH_FACTOR;
  std::vector< core::Question > questions = question_repository_->findByFilter(key.category, key.difficulty, LIMIT);
  if (!ANY_TYPE)
  {
    std::erase_if(questions, HasOtherType(key.type));
  }
  if (questions.size() > key.amount)
  {
    questions.resize(key.amount);
  }
  return questions;
}

void quiz::service::QuestionService::persistToDb(std::vector< core::Question > &questions)
{
  const std::vector< size_t > IDS = question_repository_->saveBatch(questions);
  if (IDS.size() != questions.size())
  {
    throw std::runtime_error("Question repository returned unexpected number of identifiers");
  }
  std::ranges::transform(questions, IDS, questions.begin(), IdAssigner{});
}
