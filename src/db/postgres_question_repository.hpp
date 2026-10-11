#ifndef POSTGRES_QUESTION_REPOSITORY_HPP
#define POSTGRES_QUESTION_REPOSITORY_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../core/enums.hpp"
#include "../core/question.hpp"
#include "idatabase.hpp"
#include "iquestion_repository.hpp"

namespace quiz
{
  namespace db
  {
    class PostgresQuestionRepository: public IQuestionRepository
    {
    public:
      explicit PostgresQuestionRepository(std::shared_ptr< IDatabase > database);
      std::optional< core::Question > findById(size_t id) override;
      std::vector< core::Question > findByFilter(core::Category category, core::Difficulty difficulty,
                                                 size_t limit) override;
      std::vector< core::Question > searchByText(const std::string &query, core::Category category,
                                                 core::Difficulty difficulty, size_t limit) override;
      size_t save(const core::Question &question) override;
      std::vector< size_t > saveBatch(const std::vector< core::Question > &questions) override;
      size_t count() override;

    private:
      std::shared_ptr< IDatabase > database_;
    };
  }
}

#endif
