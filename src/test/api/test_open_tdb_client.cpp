#include <array>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "api/open_tdb_client.hpp"
#include "core/enums.hpp"
#include "core/question.hpp"
#include "fake_curl.hpp"
#include <boost/test/unit_test.hpp>

namespace api = quiz::api;
namespace core = quiz::core;

BOOST_TEST_DONT_PRINT_LOG_VALUE(quiz::core::Difficulty)
BOOST_TEST_DONT_PRINT_LOG_VALUE(quiz::core::QuestionType)
BOOST_TEST_DONT_PRINT_LOG_VALUE(quiz::core::Category)

namespace
{
  constexpr std::size_t RESPONSE_LIMIT = 1024 * 1024;

  constexpr const char *GOOD_RESPONSE = R"json(
{
  "response_code": 0,
  "results": [
    {
      "type": "multiple",
      "difficulty": "hard",
      "category": "Science &amp; Nature",
      "question": "What does &quot;DNA&quot; stand for? It&#039;s a Caf&eacute; question &amp; more",
      "correct_answer": "Deoxyribonucleic acid",
      "incorrect_answers": ["Dinitro &amp; acid", "Dna&#039;s", "R&eacute;sum&eacute;"]
    },
    {
      "type": "boolean",
      "difficulty": "easy",
      "category": "Entertainment: Japanese Anime &amp; Manga",
      "question": "Naruto is an anime.",
      "correct_answer": "True",
      "incorrect_answers": ["False"]
    }
  ]
}
)json";

  class MessageContains
  {
  public:
    explicit MessageContains(std::string fragment): fragment_(std::move(fragment))
    {
    }

    bool operator()(const std::exception &error) const
    {
      return std::string(error.what()).find(fragment_) != std::string::npos;
    }

  private:
    std::string fragment_;
  };

  api::OpenTdbRequest makeRequest(size_t amount = 5, core::Category category = core::Category::ANY,
                                  core::Difficulty difficulty = core::Difficulty::ANY,
                                  core::QuestionType type = core::QuestionType::ANY)
  {
    return api::OpenTdbRequest{amount, category, difficulty, type};
  }

  std::string codeResponse(int code)
  {
    return "{\"response_code\":" + std::to_string(code) + ",\"results\":[]}";
  }

  std::string singleQuestion(const std::string &category, const std::string &difficulty, const std::string &type)
  {
    return "{\"response_code\":0,\"results\":[{\"type\":\"" + type + "\",\"difficulty\":\"" + difficulty +
           "\",\"category\":\"" + category +
           "\",\"question\":\"q\",\"correct_answer\":\"a\",\"incorrect_answers\":[\"b\"]}]}";
  }

  class FakeCurlFixture
  {
  public:
    FakeCurlFixture()
    {
      fake_curl::reset();
    }

    FakeCurlFixture(const FakeCurlFixture &) = delete;
    FakeCurlFixture &operator=(const FakeCurlFixture &) = delete;

    ~FakeCurlFixture()
    {
      BOOST_TEST(fake_curl::state().handles_opened == fake_curl::state().handles_closed);
    }

    void respondWith(const std::string &body)
    {
      fake_curl::state().response_body = body;
    }

    std::vector< core::Question > fetch(const api::OpenTdbRequest &request)
    {
      return client.fetchQuestions(request);
    }

    api::OpenTdbClient client;
  };

  struct CodeRow
  {
    int code;
    const char *fragment;
  };

  struct UnknownValueRow
  {
    const char *category;
    const char *difficulty;
    const char *type;
    const char *echoed;
  };
}

BOOST_FIXTURE_TEST_SUITE(open_tdb_client_request, FakeCurlFixture)

BOOST_AUTO_TEST_CASE(url_with_amount_only)
{
  const api::OpenTdbRequest REQUEST = makeRequest(5);
  fetch(REQUEST);
  BOOST_TEST(fake_curl::state().url == "https://opentdb.com/api.php?amount=5");
}

BOOST_AUTO_TEST_CASE(url_with_all_parameters)
{
  const api::OpenTdbRequest REQUEST =
    makeRequest(10, core::Category::SCIENCE_COMPUTERS, core::Difficulty::HARD, core::QuestionType::BOOLEAN);
  fetch(REQUEST);
  const std::string EXPECTED = "https://opentdb.com/api.php?amount=10&category=18&difficulty=hard&type=boolean";
  BOOST_TEST(fake_curl::state().url == EXPECTED);
}

BOOST_AUTO_TEST_CASE(any_values_are_not_sent)
{
  const api::OpenTdbRequest REQUEST = makeRequest(50, core::Category::ANY, core::Difficulty::MEDIUM);
  fetch(REQUEST);
  BOOST_TEST(fake_curl::state().url == "https://opentdb.com/api.php?amount=50&difficulty=medium");

  const api::OpenTdbRequest ONLY_TYPE =
    makeRequest(3, core::Category::ANY, core::Difficulty::ANY, core::QuestionType::MULTIPLE);
  fetch(ONLY_TYPE);
  BOOST_TEST(fake_curl::state().url == "https://opentdb.com/api.php?amount=3&type=multiple");
}

BOOST_AUTO_TEST_CASE(amount_limits)
{
  const api::OpenTdbRequest MINIMUM = makeRequest(1);
  const api::OpenTdbRequest MAXIMUM = makeRequest(50);
  BOOST_CHECK_NO_THROW(fetch(MINIMUM));
  BOOST_TEST(fake_curl::state().url == "https://opentdb.com/api.php?amount=1");
  BOOST_CHECK_NO_THROW(fetch(MAXIMUM));
  BOOST_TEST(fake_curl::state().url == "https://opentdb.com/api.php?amount=50");

  const int OPENED_BEFORE = fake_curl::state().handles_opened;
  const api::OpenTdbRequest TOO_FEW = makeRequest(0);
  const api::OpenTdbRequest TOO_MANY = makeRequest(51);
  BOOST_CHECK_THROW(fetch(TOO_FEW), std::invalid_argument);
  BOOST_CHECK_THROW(fetch(TOO_MANY), std::invalid_argument);
  BOOST_TEST(fake_curl::state().handles_opened == OPENED_BEFORE);
}

BOOST_AUTO_TEST_CASE(transport_options)
{
  const api::OpenTdbRequest REQUEST = makeRequest();
  fetch(REQUEST);
  BOOST_TEST(fake_curl::state().user_agent == "terminal-quiz/1.0");
  BOOST_TEST(fake_curl::state().timeout == 10L);
  BOOST_TEST(fake_curl::state().no_signal == 1L);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(open_tdb_client_response, FakeCurlFixture)

BOOST_AUTO_TEST_CASE(maps_fields_and_decodes_html)
{
  respondWith(GOOD_RESPONSE);
  const api::OpenTdbRequest REQUEST = makeRequest(2);
  const std::vector< core::Question > QUESTIONS = fetch(REQUEST);
  BOOST_REQUIRE(QUESTIONS.size() == 2U);

  BOOST_TEST(QUESTIONS[0].id == 0U);
  BOOST_TEST(QUESTIONS[0].category == core::Category::SCIENCE_NATURE);
  BOOST_TEST(QUESTIONS[0].difficulty == core::Difficulty::HARD);
  BOOST_TEST(QUESTIONS[0].type == core::QuestionType::MULTIPLE);
  BOOST_TEST(QUESTIONS[0].text == "What does \"DNA\" stand for? It's a Caf\xC3\xA9 question & more");
  BOOST_TEST(QUESTIONS[0].correct_answer == "Deoxyribonucleic acid");
  BOOST_REQUIRE(QUESTIONS[0].incorrect_answers.size() == 3U);
  BOOST_TEST(QUESTIONS[0].incorrect_answers[0] == "Dinitro & acid");
  BOOST_TEST(QUESTIONS[0].incorrect_answers[1] == "Dna's");
  BOOST_TEST(QUESTIONS[0].incorrect_answers[2] == "R\xC3\xA9sum\xC3\xA9");

  BOOST_TEST(QUESTIONS[1].id == 0U);
  BOOST_TEST(QUESTIONS[1].category == core::Category::ENTERTAINMENT_ANIME);
  BOOST_TEST(QUESTIONS[1].difficulty == core::Difficulty::EASY);
  BOOST_TEST(QUESTIONS[1].type == core::QuestionType::BOOLEAN);
  BOOST_TEST(QUESTIONS[1].text == "Naruto is an anime.");
  BOOST_TEST(QUESTIONS[1].correct_answer == "True");
  BOOST_REQUIRE(QUESTIONS[1].incorrect_answers.size() == 1U);
  BOOST_TEST(QUESTIONS[1].incorrect_answers[0] == "False");
}

BOOST_AUTO_TEST_CASE(no_results_code_and_empty_array_return_no_questions)
{
  const api::OpenTdbRequest REQUEST = makeRequest(5, core::Category::ART, core::Difficulty::HARD);
  respondWith(codeResponse(1));
  BOOST_TEST(fetch(REQUEST).empty());
  respondWith(codeResponse(0));
  BOOST_TEST(fetch(REQUEST).empty());
}

BOOST_AUTO_TEST_CASE(error_codes_throw_with_reason)
{
  const std::array< CodeRow, 5 > ROWS = std::to_array< CodeRow >({
    {2, "rejected the request parameters"},
    {3, "token not found"},
    {4, "all possible questions"},
    {5, "rate limit"},
    {9, "unknown response code 9"},
  });
  const api::OpenTdbRequest REQUEST = makeRequest();
  for (const CodeRow &row : ROWS)
  {
    BOOST_TEST_CONTEXT("response_code " << row.code)
    {
      respondWith(codeResponse(row.code));
      BOOST_CHECK_EXCEPTION(fetch(REQUEST), std::runtime_error, MessageContains(row.fragment));
    }
  }
}

BOOST_AUTO_TEST_CASE(consecutive_requests_are_independent)
{
  const api::OpenTdbRequest REQUEST = makeRequest(2);
  respondWith(GOOD_RESPONSE);
  BOOST_TEST(fetch(REQUEST).size() == 2U);
  respondWith(codeResponse(0));
  BOOST_TEST(fetch(REQUEST).empty());
  respondWith(GOOD_RESPONSE);
  BOOST_TEST(fetch(REQUEST).size() == 2U);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(open_tdb_client_transport, FakeCurlFixture)

BOOST_AUTO_TEST_CASE(http_errors)
{
  const api::OpenTdbRequest REQUEST = makeRequest();
  respondWith(GOOD_RESPONSE);

  fake_curl::state().http_status = 429;
  BOOST_CHECK_EXCEPTION(fetch(REQUEST), std::runtime_error, MessageContains("rate limit"));
  fake_curl::state().http_status = 500;
  BOOST_CHECK_EXCEPTION(fetch(REQUEST), std::runtime_error, MessageContains("HTTP status 500"));
  fake_curl::state().http_status = 404;
  BOOST_CHECK_EXCEPTION(fetch(REQUEST), std::runtime_error, MessageContains("HTTP status 404"));
}

BOOST_AUTO_TEST_CASE(curl_errors)
{
  const api::OpenTdbRequest REQUEST = makeRequest();
  fake_curl::state().transfer_result = fake_curl::TRANSFER_COULDNT_CONNECT;
  BOOST_CHECK_EXCEPTION(fetch(REQUEST), std::runtime_error, MessageContains("Couldn't connect"));
  fake_curl::state().transfer_result = fake_curl::TRANSFER_TIMEOUT;
  BOOST_CHECK_EXCEPTION(fetch(REQUEST), std::runtime_error, MessageContains("Timeout"));
}

BOOST_AUTO_TEST_CASE(body_size_limit)
{
  const api::OpenTdbRequest REQUEST = makeRequest();
  respondWith(std::string(RESPONSE_LIMIT + 1, 'x'));
  BOOST_CHECK_EXCEPTION(fetch(REQUEST), std::runtime_error, MessageContains("Request to Open Trivia DB failed"));

  respondWith(std::string(RESPONSE_LIMIT, 'x'));
  BOOST_CHECK_EXCEPTION(fetch(REQUEST), std::runtime_error, MessageContains("Malformed"));
}

BOOST_AUTO_TEST_CASE(handle_creation_failure)
{
  const api::OpenTdbRequest REQUEST = makeRequest();
  fake_curl::state().fail_init = true;
  BOOST_CHECK_EXCEPTION(fetch(REQUEST), std::runtime_error, MessageContains("Failed to create libcurl handle"));
  BOOST_TEST(fake_curl::state().performs == 0);
}

BOOST_AUTO_TEST_CASE(option_setup_failure_releases_handle)
{
  const api::OpenTdbRequest REQUEST = makeRequest();
  for (int failing_call = 1; failing_call <= 6; ++failing_call)
  {
    BOOST_TEST_CONTEXT("setopt call " << failing_call << " fails")
    {
      fake_curl::state().setopt_calls = 0;
      fake_curl::state().fail_setopt_at = failing_call;
      BOOST_CHECK_EXCEPTION(fetch(REQUEST), std::runtime_error, MessageContains("Failed to configure libcurl"));
      BOOST_TEST(fake_curl::state().handles_opened == fake_curl::state().handles_closed);
      BOOST_TEST(fake_curl::state().performs == 0);
    }
  }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(open_tdb_client_malformed_data, FakeCurlFixture)

BOOST_AUTO_TEST_CASE(malformed_json_and_missing_fields_throw)
{
  const std::array< const char *, 13 > PAYLOADS = std::to_array< const char * >({
    "",
    "not json",
    R"json({"response_code":0,"results":[]} trailing)json",
    R"json({"response_code":0,"results":[)json",
    R"json({"results":[]})json",
    R"json({"response_code":0})json",
    R"json({"response_code":"0","results":[]})json",
    R"json({"response_code":0,"results":{}})json",
    R"json({"response_code":0,"results":[null]})json",
    R"json({"response_code":0,"results":[{"type":"multiple"}]})json",
    R"json({"response_code":0,"results":[{"type":"multiple","difficulty":"easy","category":"Art",
            "question":5,"correct_answer":"a","incorrect_answers":["b"]}]})json",
    R"json({"response_code":0,"results":[{"type":"multiple","difficulty":"easy","category":"Art",
            "question":"q","correct_answer":"a","incorrect_answers":[1]}]})json",
    R"json({"response_code":0,"results":[{"type":"multiple","difficulty":"easy","category":"Art",
            "question":"q","correct_answer":"a"}]})json",
  });
  const api::OpenTdbRequest REQUEST = makeRequest();
  for (const char *payload : PAYLOADS)
  {
    BOOST_TEST_CONTEXT("payload: " << payload)
    {
      respondWith(payload);
      BOOST_CHECK_EXCEPTION(fetch(REQUEST), std::runtime_error, MessageContains("Malformed Open Trivia DB response"));
    }
  }
}

BOOST_AUTO_TEST_CASE(unknown_enum_values_throw_with_the_value)
{
  const std::array< UnknownValueRow, 4 > ROWS = std::to_array< UnknownValueRow >({
    {"No Such Category", "easy", "multiple", "No Such Category"},
    {"Science &amp;amp; Nature", "easy", "multiple", "Science &amp; Nature"},
    {"Art", "impossible", "multiple", "impossible"},
    {"Art", "easy", "single", "single"},
  });
  const api::OpenTdbRequest REQUEST = makeRequest();
  for (const UnknownValueRow &row : ROWS)
  {
    BOOST_TEST_CONTEXT(row.echoed)
    {
      respondWith(singleQuestion(row.category, row.difficulty, row.type));
      BOOST_CHECK_EXCEPTION(fetch(REQUEST), std::runtime_error, MessageContains("Unexpected value"));
      BOOST_CHECK_EXCEPTION(fetch(REQUEST), std::runtime_error, MessageContains(row.echoed));
    }
  }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(open_tdb_client_lifetime, FakeCurlFixture)

BOOST_AUTO_TEST_CASE(libcurl_is_initialized_once)
{
  const int INITIALIZED = fake_curl::state().global_init_calls;
  BOOST_TEST(INITIALIZED >= 1);
  {
    api::OpenTdbClient second;
    api::OpenTdbClient third;
  }
  BOOST_TEST(fake_curl::state().global_init_calls == INITIALIZED);
  BOOST_TEST(fake_curl::state().global_cleanup_calls == 0);
}

BOOST_AUTO_TEST_CASE(usable_through_interface)
{
  respondWith(GOOD_RESPONSE);
  const std::unique_ptr< api::IOpenTdbClient > INTERFACE_POINTER = std::make_unique< api::OpenTdbClient >();
  const api::OpenTdbRequest REQUEST = makeRequest(2);
  BOOST_TEST(INTERFACE_POINTER->fetchQuestions(REQUEST).size() == 2U);
}

BOOST_AUTO_TEST_SUITE_END()
