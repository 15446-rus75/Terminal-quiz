#ifndef POSTGRES_CUSTOM_TEST_REPOSITORY_HPP
#define POSTGRES_CUSTOM_TEST_REPOSITORY_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "icustom_test_repository.hpp"
#include "idatabase.hpp"

namespace quiz
{
  namespace db
  {
    class PostgresCustomTestRepository: public ICustomTestRepository
    {
    public:
      explicit PostgresCustomTestRepository(std::shared_ptr< IDatabase > database);
      size_t save(const core::CustomTest &test) override;
      std::optional< core::CustomTest > findById(size_t id) override;
      std::vector< core::CustomTest > findByUserId(size_t user_id) override;
      void remove(size_t id) override;

    private:
      void saveQuestions(size_t test_id, const std::vector< size_t > &question_ids);

      std::shared_ptr< IDatabase > database_;
    };
  }
}

#endif
