#ifndef POSTGRES_QUESTION_REPOSITORY_HPP
#define POSTGRES_QUESTION_REPOSITORY_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

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
      void save(const core::Question &question) override;
      void saveBatch(const std::vector< core::Question > &questions) override;
      size_t count() override;

    private:
      core::Question mapRow(/*WIP*/) const;

      std::shared_ptr< IDatabase > database_;
    };
  }
}

#endif
