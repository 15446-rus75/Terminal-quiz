#ifndef CUSTOM_TEST_SCREEN_HPP
#define CUSTOM_TEST_SCREEN_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../core/custom_test.hpp"
#include "../core/question.hpp"
#include "../services/custom_test_service.hpp"
#include "../services/question_service.hpp"
#include "forms.hpp"
#include "prompter.hpp"
#include "quiz_runner.hpp"
#include "renderer.hpp"
#include "screen.hpp"
#include "terminal.hpp"

namespace quiz
{
  namespace tui
  {
    class CustomTestScreen: public IScreen
    {
    public:
      CustomTestScreen(std::shared_ptr< ITerminal > terminal,
                       std::shared_ptr< service::CustomTestService > custom_test_service,
                       std::shared_ptr< service::QuestionService > question_service,
                       std::shared_ptr< QuizRunner > runner, size_t user_id);
      std::string title() const override;
      void run() override;

    private:
      void createTest();
      void playTest();
      void deleteTest();
      std::optional< size_t > chooseTest(const std::vector< core::CustomTest > &tests);
      std::vector< core::Question > pickByFilter();
      std::vector< core::Question > pickFromSearch();

      Renderer renderer_;
      Forms forms_;
      Prompter prompter_;
      std::shared_ptr< service::CustomTestService > custom_test_service_;
      std::shared_ptr< service::QuestionService > question_service_;
      std::shared_ptr< QuizRunner > runner_;
      size_t user_id_;
    };
  }
}

#endif
