#ifndef QUIZ_SCREEN_HPP
#define QUIZ_SCREEN_HPP

#include <memory>
#include <string>

#include "../services/question_service.hpp"
#include "forms.hpp"
#include "quiz_runner.hpp"
#include "renderer.hpp"
#include "screen.hpp"
#include "terminal.hpp"

namespace quiz
{
  namespace tui
  {
    class QuizScreen: public IScreen
    {
    public:
      QuizScreen(std::shared_ptr< ITerminal > terminal, std::shared_ptr< service::QuestionService > question_service,
                 std::shared_ptr< QuizRunner > runner);
      std::string title() const override;
      void run() override;

    private:
      Renderer renderer_;
      Forms forms_;
      std::shared_ptr< service::QuestionService > question_service_;
      std::shared_ptr< QuizRunner > runner_;
    };
  }
}

#endif
