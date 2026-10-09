#ifndef QUESTION_SERVICE_HPP
#define QUESTION_SERVICE_HPP

#include "../api/open_tdb_client.hpp"
#include "../core/enums.hpp"
#include "../core/question.hpp"
#include "../db/i_question_repository.hpp"
#include "../util/lru_cache.hpp"
#include <cstdint>
#include <memory>
#include <vector>

namespace quiz
{
  namespace service
  {
    class QuestionService
    {
    public:
      QuestionService(std::shared_ptr< api::IOpenTdbClient > open_tdb_client,
                      std::shared_ptr< db::IQuestionRepository > question_repository, size_t cache_capacity);
      std::vector< core::Question > fetchQuestions(size_t amount, core::Category category, core::Difficulty difficulty,
                                                   core::QuestionType type);
      std::vector< core::Question > searchByText(const std::string &query, core::Category category,
                                                 core::Difficulty difficulty, size_t limit);
      std::optional< core::Question > findById(size_t id);

    private:
      struct CacheKey
      {
        size_t amount;
        core::Category category;
        core::Difficulty difficulty;
        core::QuestionType type;
        bool operator==(const CacheKey &other) const = default;
      };

      struct CacheKeyHash
      {
        size_t operator()(const CacheKey &key) const;
      };

      std::vector< core::Question > fetchFromApi(const CacheKey &key);
      std::vector< core::Question > fetchFromDb(const CacheKey &key);
      void persistToDb(const std::vector< core::Question > &questions);

      std::shared_ptr< api::IOpenTdbClient > open_tdb_client_;
      std::shared_ptr< db::IQuestionRepository > question_repository_;
      util::LruCache< CacheKey, std::vector< core::Question > > cache_;
    };
  }
}

#endif
