#include <algorithm>
#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/custom_test.hpp"
#include "core/question.hpp"
#include "db/icustom_test_repository.hpp"
#include "db/iquestion_repository.hpp"
#include "services/custom_test_service.hpp"
#include <boost/test/unit_test.hpp>

namespace core = quiz::core;
namespace db = quiz::db;
namespace service = quiz::service;

namespace
{
  class MockCustomTestRepository: public db::ICustomTestRepository
  {
  public:
    size_t save(const core::CustomTest &test) override
    {
      last_saved = test;
      last_saved->id = ++next_id;
      tests.push_back(*last_saved);
      return last_saved->id;
    }

    std::optional< core::CustomTest > findById(size_t id) override
    {
      for (const auto &test : tests)
      {
        if (test.id == id)
        {
          return test;
        }
      }
      return std::nullopt;
    }

    std::vector< core::CustomTest > findByUserId(size_t user_id) override
    {
      std::vector< core::CustomTest > result;
      for (const auto &test : tests)
      {
        if (test.user_id == user_id)
        {
          result.push_back(test);
        }
      }
      return result;
    }

    void remove(size_t id) override
    {
      std::erase_if(tests,
                    [id](const core::CustomTest &t)
                    {
                      return t.id == id;
                    });
    }

    size_t next_id = 0;
    std::optional< core::CustomTest > last_saved;
    std::vector< core::CustomTest > tests;
  };

  class MockQuestionRepository: public db::IQuestionRepository
  {
  public:
    size_t save(const core::Question &) override
    {
      return 0;
    }

    std::vector< size_t > saveBatch(const std::vector< core::Question > &) override
    {
      return {};
    }

    std::optional< core::Question > findById(size_t id) override
    {
      for (const auto &q : questions)
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
      return {};
    }

    std::vector< core::Question > searchByText(const std::string &, core::Category, core::Difficulty, size_t) override
    {
      return {};
    }

    size_t count() override
    {
      return questions.size();
    }

    std::vector< core::Question > questions;
  };

  core::Question makeQuestion(size_t id)
  {
    core::Question q;
    q.id = id;
    q.category = core::Category::GENERAL_KNOWLEDGE;
    q.difficulty = core::Difficulty::EASY;
    q.type = core::QuestionType::MULTIPLE;
    q.text = "Question " + std::to_string(id);
    q.correct_answer = "A";
    q.incorrect_answers = {"B", "C", "D"};
    return q;
  }
}

BOOST_AUTO_TEST_SUITE(custom_test_service_constructor)

BOOST_AUTO_TEST_CASE(null_repositories_throw)
{
  auto test_repo = std::make_shared< MockCustomTestRepository >();
  auto q_repo = std::make_shared< MockQuestionRepository >();

  BOOST_CHECK_THROW(service::CustomTestService(nullptr, q_repo), std::invalid_argument);
  BOOST_CHECK_THROW(service::CustomTestService(test_repo, nullptr), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(custom_test_service_create)

BOOST_AUTO_TEST_CASE(create_valid_test)
{
  auto test_repo = std::make_shared< MockCustomTestRepository >();
  auto q_repo = std::make_shared< MockQuestionRepository >();
  q_repo->questions = {makeQuestion(10), makeQuestion(20)};

  service::CustomTestService service(test_repo, q_repo);
  const size_t ID = service.create(1, "My Test", {10, 20});

  BOOST_TEST(ID == 1U);
  BOOST_TEST(test_repo->last_saved.has_value());
  BOOST_TEST(test_repo->last_saved->name == "My Test");
  BOOST_TEST(test_repo->last_saved->user_id == 1U);
}

BOOST_AUTO_TEST_CASE(empty_name_throws)
{
  auto test_repo = std::make_shared< MockCustomTestRepository >();
  auto q_repo = std::make_shared< MockQuestionRepository >();

  service::CustomTestService service(test_repo, q_repo);
  BOOST_CHECK_THROW(service.create(1, "", {10}), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(empty_questions_throws)
{
  auto test_repo = std::make_shared< MockCustomTestRepository >();
  auto q_repo = std::make_shared< MockQuestionRepository >();

  service::CustomTestService service(test_repo, q_repo);
  BOOST_CHECK_THROW(service.create(1, "Test", {}), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(duplicate_questions_throw)
{
  auto test_repo = std::make_shared< MockCustomTestRepository >();
  auto q_repo = std::make_shared< MockQuestionRepository >();

  service::CustomTestService service(test_repo, q_repo);
  BOOST_CHECK_THROW(service.create(1, "Test", {10, 20, 10}), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(missing_question_throws)
{
  auto test_repo = std::make_shared< MockCustomTestRepository >();
  auto q_repo = std::make_shared< MockQuestionRepository >();
  q_repo->questions = {makeQuestion(10)};

  service::CustomTestService service(test_repo, q_repo);
  BOOST_CHECK_THROW(service.create(1, "Test", {10, 999}), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(custom_test_service_queries)

BOOST_AUTO_TEST_CASE(find_by_id_and_user_id)
{
  auto test_repo = std::make_shared< MockCustomTestRepository >();
  auto q_repo = std::make_shared< MockQuestionRepository >();
  q_repo->questions = {makeQuestion(1)};

  service::CustomTestService service(test_repo, q_repo);
  const size_t ID = service.create(42, "User Test", {1});

  const auto FOUND = service.findById(ID);
  BOOST_TEST(FOUND.has_value());
  BOOST_TEST(FOUND->name == "User Test");

  const auto USER_TESTS = service.findByUserId(42);
  BOOST_TEST(USER_TESTS.size() == 1U);

  service.remove(ID);
  BOOST_TEST(!service.findById(ID).has_value());
}

BOOST_AUTO_TEST_CASE(load_questions_success_and_failure)
{
  auto test_repo = std::make_shared< MockCustomTestRepository >();
  auto q_repo = std::make_shared< MockQuestionRepository >();
  q_repo->questions = {makeQuestion(1), makeQuestion(2)};

  service::CustomTestService service(test_repo, q_repo);
  const size_t ID = service.create(1, "Test", {1, 2});

  const auto LOADED = service.loadQuestions(ID);
  BOOST_TEST(LOADED.size() == 2U);
  BOOST_TEST(LOADED[0].id == 1U);
  BOOST_TEST(LOADED[1].id == 2U);

  BOOST_CHECK_THROW(service.loadQuestions(999), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()
