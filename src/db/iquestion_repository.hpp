#ifndef IQUESTION_REPOSITORY_HPP
#define IQUESTION_REPOSITORY_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "../core/enums.hpp"
#include "../core/question.hpp"

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
      virtual size_t save(const core::Question &question) = 0;
      virtual std::vector< size_t > saveBatch(const std::vector< core::Question > &questions) = 0;
      virtual size_t count() = 0;
    };
  }
}

#endif
