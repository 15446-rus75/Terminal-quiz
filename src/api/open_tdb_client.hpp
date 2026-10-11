#ifndef OPEN_TDB_CLIENT_HPP
#define OPEN_TDB_CLIENT_HPP

#include <cstddef>
#include <string>
#include <vector>

#include "../core/enums.hpp"
#include "../core/question.hpp"

namespace quiz
{
  namespace api
  {
    struct OpenTdbRequest
    {
      size_t amount;
      core::Category category;
      core::Difficulty difficulty;
      core::QuestionType type;
    };

    class IOpenTdbClient
    {
    public:
      virtual ~IOpenTdbClient() = default;
      virtual std::vector< core::Question > fetchQuestions(const OpenTdbRequest &request) = 0;
    };

    class OpenTdbClient: public IOpenTdbClient
    {
    public:
      OpenTdbClient();
      ~OpenTdbClient() override;
      std::vector< core::Question > fetchQuestions(const OpenTdbRequest &request) override;

    private:
      std::string buildUrl(const OpenTdbRequest &request) const;
      std::string performGet(const std::string &url);
      std::vector< core::Question > parseResponse(const std::string &json);

      static constexpr const char *BASE_URL = "https://opentdb.com/api.php";
      static constexpr size_t TIMEOUT_SECONDS = 10;
    };
  }
}

#endif
