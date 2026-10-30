#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "api/open_tdb_client.hpp"
#include "core/enums.hpp"
#include "core/question.hpp"
#include "db/iquestion_repository.hpp"
#include "services/question_service.hpp"
#include <boost/test/unit_test.hpp>

namespace core = quiz::core;
namespace db = quiz::db;
namespace api = quiz::api;
namespace service = quiz::service;

namespace
{
  class MockOpenTdbClient: public api::IOpenTdbClient
  {
  public:
    std::vector< core::Question > fetchQuestions(const api::OpenTdbRequest &req) override
    {
      if (should_throw)
      {
        throw std::runtime_error("API Error");
      }
      return api_questions;
    }

    bool should_throw = false;
    std::vector< core::Question > api_questions;
  };

  class MockQuestionRepository: public db::IQuestionRepository
  {
  public:
    size_t save(const core::Question &) override
    {
      return 0;
    }

    std::vector< size_t > saveBatch(const std::vector< core::Question > &questions) override
    {
      if (fail_batch_save)
      {
        return {};
      }
      std::vector< size_t > ids;
      for (size_t i = 0; i < questions.size(); ++i)
      {
        ids.push_back(i + 1);
      }
      return ids;
    }

    std::optional< core::Question > findById(size_t id) override
    {
      for (const auto &q: db_questions)
      {
        if (q.id == id)
        {
          return q;
        }
      }
      return std::nullopt;
    }

    std::vector< core::Question > findByFilter(core::Category, core::Difficulty, size_t) override
    {
      return db_questions;
    }

    std::vector< core::Question > searchByText(const std::string &, core::Category, core::Difficulty, size_t limit) override
    {
      std::vector< core::Question > res = db_questions;
      if (res.size() > limit)
      {
        res.resize(limit);
      }
      return res;
    }

    bool fail_batch_save = false;
    std::vector< core::Question > db_questions;
  };

  core::Question makeQuestion(size_t id, core::QuestionType type = core::QuestionType::MULTIPLE)
  {
    core::Question q;
    q.id = id;
    q.category = core::Category::GENERAL_KNOWLEDGE;
    q.difficulty = core::Difficulty::EASY;
    q.type = type;
    q.text = "Q" + std::to_string(id);
    return q;
  }
}

BOOST_AUTO_TEST_SUITE(question_service_constructor)

BOOST_AUTO_TEST_CASE(null_dependencies_throw)
{
  auto client = std::make_shared< MockOpenTdbClient >();
  auto repo = std::make_shared< MockQuestionRepository >();

  BOOST_CHECK_THROW(service::QuestionService(nullptr, repo), std::invalid_argument);
  BOOST_CHECK_THROW(service::QuestionService(client, nullptr), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(question_service_fetch_questions)

BOOST_AUTO_TEST_CASE(invalid_amount_throws)
{
  auto client = std::make_shared< MockOpenTdbClient >();
  auto repo = std::make_shared< MockQuestionRepository >();
  service::QuestionService service(client, repo);

  BOOST_CHECK_THROW(service.fetchQuestions(0, core::Category::ANY, core::Difficulty::ANY, core::QuestionType::ANY),
                    std::invalid_argument);
  BOOST_CHECK_THROW(service.fetchQuestions(51, core::Category::ANY, core::Difficulty::ANY, core::QuestionType::ANY),
                    std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(fetch_from_api_and_cache)
{
  auto client = std::make_shared< MockOpenTdbClient >();
  auto repo = std::make_shared< MockQuestionRepository >();
  client->api_questions = {makeQuestion(0)};

  service::QuestionService service(client, repo);

  const auto RESULT1 = service.fetchQuestions(1, core::Category::ANY, core::Difficulty::ANY, core::QuestionType::ANY);
  BOOST_TEST(RESULT1.size() == 1U);
  BOOST_TEST(RESULT1[0].id == 1U);

  client->api_questions.clear();
  const auto RESULT2 = service.fetchQuestions(1, core::Category::ANY, core::Difficulty::ANY, core::QuestionType::ANY);
  BOOST_TEST(RESULT2.size() == 1U);
  BOOST_TEST(RESULT2[0].id == 1U);
}

BOOST_AUTO_TEST_CASE(fallback_to_db_when_api_fails)
{
  auto client = std::make_shared< MockOpenTdbClient >();
  auto repo = std::make_shared< MockQuestionRepository >();
  client->should_throw = true;
  repo->db_questions = {makeQuestion(10)};

  service::QuestionService service(client, repo);

  const auto RESULT = service.fetchQuestions(1, core::Category::ANY, core::Difficulty::ANY, core::QuestionType::ANY);
  BOOST_TEST(RESULT.size() == 1U);
  BOOST_TEST(RESULT[0].id == 10U);
}

BOOST_AUTO_TEST_CASE(fallback_to_db_when_api_fails_and_db_empty_throws)
{
  auto client = std::make_shared< MockOpenTdbClient >();
  auto repo = std::make_shared< MockQuestionRepository >();
  client->should_throw = true;

  service::QuestionService service(client, repo);

  BOOST_CHECK_THROW(service.fetchQuestions(1, core::Category::ANY, core::Difficulty::ANY, core::QuestionType::ANY),
                    std::runtime_error);
}

BOOST_AUTO_TEST_CASE(fallback_to_db_when_api_returns_empty)
{
  auto client = std::make_shared< MockOpenTdbClient >();
  auto repo = std::make_shared< MockQuestionRepository >();
  repo->db_questions = {makeQuestion(5)};

  service::QuestionService service(client, repo);

  const auto RESULT = service.fetchQuestions(1, core::Category::ANY, core::Difficulty::ANY, core::QuestionType::ANY);
  BOOST_TEST(RESULT.size() == 1U);
  BOOST_TEST(RESULT[0].id == 5U);
}

BOOST_AUTO_TEST_CASE(persist_to_db_id_mismatch_throws)
{
  auto client = std::make_shared< MockOpenTdbClient >();
  auto repo = std::make_shared< MockQuestionRepository >();
  client->api_questions = {makeQuestion(0)};
  repo->fail_batch_save = true;

  service::QuestionService service(client, repo);

  BOOST_CHECK_THROW(service.fetchQuestions(1, core::Category::ANY, core::Difficulty::ANY, core::QuestionType::ANY),
                    std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(question_service_search_and_find)

BOOST_AUTO_TEST_CASE(search_by_text_zero_limit_returns_empty)
{
  auto client = std::make_shared< MockOpenTdbClient >();
  auto repo = std::make_shared< MockQuestionRepository >();

  service::QuestionService service(client, repo);
  BOOST_TEST(service.searchByText("query", core::Category::ANY, core::Difficulty::ANY, 0).empty());
}

BOOST_AUTO_TEST_CASE(search_by_text_and_find_by_id)
{
  auto client = std::make_shared< MockOpenTdbClient >();
  auto repo = std::make_shared< MockQuestionRepository >();
  repo->db_questions = {makeQuestion(42)};

  service::QuestionService service(client, repo);

  const auto SEARCH_RES = service.searchByText("query", core::Category::ANY, core::Difficulty::ANY, 5);
  BOOST_TEST(SEARCH_RES.size() == 1U);

  const auto FIND_RES = service.findById(42);
  BOOST_TEST(FIND_RES.has_value());
  BOOST_TEST(FIND_RES->id == 42U);
}

BOOST_AUTO_TEST_SUITE_END()
