#include "open_tdb_client.hpp"

#include <cstddef>
#include <curl/curl.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "../core/enums.hpp"
#include "../core/question.hpp"
#include "../util/html_decoder.hpp"
#include "../util/logger.hpp"
#include <nlohmann/json.hpp>

namespace
{
  constexpr size_t MIN_AMOUNT = 1;
  constexpr size_t MAX_AMOUNT = 50;
  constexpr size_t MAX_RESPONSE_BYTES = static_cast< size_t >(1024 * 1024);
  constexpr long NO_SIGNAL = 1;
  constexpr long HTTP_OK = 200;
  constexpr long HTTP_TOO_MANY_REQUESTS = 429;
  constexpr const char *USER_AGENT = "terminal-quiz/1.0";

  constexpr int RESPONSE_SUCCESS = 0;
  constexpr int RESPONSE_NO_RESULTS = 1;
  constexpr int RESPONSE_INVALID_PARAMETER = 2;
  constexpr int RESPONSE_TOKEN_NOT_FOUND = 3;
  constexpr int RESPONSE_TOKEN_EMPTY = 4;
  constexpr int RESPONSE_RATE_LIMIT = 5;

  class CurlGlobalState
  {
  public:
    CurlGlobalState();
    CurlGlobalState(const CurlGlobalState &) = delete;
    CurlGlobalState &operator=(const CurlGlobalState &) = delete;
    CurlGlobalState(CurlGlobalState &&) = delete;
    CurlGlobalState &operator=(CurlGlobalState &&) = delete;
    ~CurlGlobalState();
  };

  CurlGlobalState::CurlGlobalState()
  {
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
    {
      throw std::runtime_error("Failed to initialize libcurl");
    }
  }

  CurlGlobalState::~CurlGlobalState()
  {
    curl_global_cleanup();
  }

  struct CurlDeleter
  {
    void operator()(CURL *handle) const
    {
      curl_easy_cleanup(handle);
    }
  };

  size_t writeBody(char *data, size_t size, size_t count, void *target)
  {
    std::string &body = *static_cast< std::string * >(target);
    const size_t BYTES = size * count;
    if (BYTES > MAX_RESPONSE_BYTES - body.size())
    {
      return 0;
    }
    body.append(data, BYTES);
    return BYTES;
  }

  template < class Value >
  void setOption(CURL *handle, CURLoption option, Value value)
  {
    if (curl_easy_setopt(handle, option, value) != CURLE_OK)
    {
      throw std::runtime_error("Failed to configure libcurl");
    }
  }

  std::string describeResponseCode(int code)
  {
    switch (code)
    {
    case RESPONSE_INVALID_PARAMETER:
      return "Open Trivia DB rejected the request parameters";
    case RESPONSE_TOKEN_NOT_FOUND:
      return "Open Trivia DB session token not found";
    case RESPONSE_TOKEN_EMPTY:
      return "Open Trivia DB session token has returned all possible questions";
    case RESPONSE_RATE_LIMIT:
      return "Open Trivia DB rate limit exceeded (one request per 5 seconds per IP address)";
    default:
      return "Open Trivia DB returned unknown response code " + std::to_string(code);
    }
  }

  quiz::core::Question parseQuestion(const nlohmann::json &entry)
  {
    const std::string CATEGORY = quiz::util::HtmlDecoder::decode(entry.at("category").get< std::string >());
    const std::string DIFFICULTY = entry.at("difficulty").get< std::string >();
    const std::string TYPE = entry.at("type").get< std::string >();

    quiz::core::Question question{};
    try
    {
      question.category = quiz::core::parseCategory(CATEGORY);
      question.difficulty = quiz::core::parseDifficulty(DIFFICULTY);
      question.type = quiz::core::parseQuestionType(TYPE);
    }
    catch (const std::invalid_argument &error)
    {
      throw std::runtime_error("Unexpected value in Open Trivia DB response (category \"" + CATEGORY +
                               "\", difficulty \"" + DIFFICULTY + "\", type \"" + TYPE + "\"): " + error.what());
    }

    question.text = quiz::util::HtmlDecoder::decode(entry.at("question").get< std::string >());
    question.correct_answer = quiz::util::HtmlDecoder::decode(entry.at("correct_answer").get< std::string >());
    for (const nlohmann::json &answer : entry.at("incorrect_answers"))
    {
      question.incorrect_answers.push_back(quiz::util::HtmlDecoder::decode(answer.get< std::string >()));
    }
    return question;
  }
}
