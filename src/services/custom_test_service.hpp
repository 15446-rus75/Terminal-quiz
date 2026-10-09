#ifndef CUSTOM_TEST_SERVICE_HPP
#define CUSTOM_TEST_SERVICE_HPP

#include "../core/custom_test.hpp"
#include "../core/question.hpp"
#include "../db/i_custom_test_repository.hpp"
#include "../db/i_question_repository.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace quiz
{
  namespace service
  {
    class CustomTestService
    {
    public:
      CustomTestService(std::shared_ptr< db::ICustomTestRepository > custom_test_repository,
                        std::shared_ptr< db::IQuestionRepository > question_repository);
      size_t create(size_t user_id, const std::string &name, const std::vector< size_t > &question_ids);
      std::optional< core::CustomTest > findById(size_t id);
      std::vector< core::CustomTest > findByUserId(siez_t user_id);
      void remove(size_t id);
      std::vector< core::Question > loadQuestions(size_t test_id);

    private:
      std::shared_ptr< db::ICustomTestRepository > custom_test_repository_;
      std::shared_ptr< db::IQuestionRepository > question_repository_;
    };
  }
}

#endif
