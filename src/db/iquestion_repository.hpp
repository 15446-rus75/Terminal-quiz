#ifndef IQUESTION_REPOSITORY_HPP
#define IQUESTION_REPOSITORY_HPP

#include "../core/enums.hpp"
#include "../core/question.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace quiz
{
  namespace db
  {
    class IQuestionRepository
    {
    public:
      virtual ~IQuestionRepository() = default;
      virtual std::optional< core::Question > findById(size_t id) = 0;
      virtual std::vector< core::Question > findByFilter(core::Category category, core::Difficulty difficulty,
                                                         size_t limit) = 0;
      virtual std::vector< core::Question > searchByText(const std::string &query, core::Category category,
                                                         core::Difficulty difficulty, size_t limit) = 0;
      virtual void save(const core::Question &question) = 0;
      virtual void saveBatch(const std::vector< core::Question > &questions) = 0;
      virtual size_t count() = 0;
    };
  }
}

#endif
