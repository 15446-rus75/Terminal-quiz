#ifndef SEARCH_SCREEN_HPP
#define SEARCH_SCREEN_HPP

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "../core/question.hpp"
#include "../services/favorite_service.hpp"
#include "../services/question_service.hpp"
#include "forms.hpp"
#include "prompter.hpp"
#include "renderer.hpp"
#include "screen.hpp"
#include "terminal.hpp"

namespace quiz
{
  namespace tui
  {
    class SearchScreen: public IScreen
    {
    public:
      SearchScreen(std::shared_ptr< ITerminal > terminal, std::shared_ptr< service::QuestionService > question_service,
                   std::shared_ptr< service::FavoriteService > favorite_service, size_t user_id);
      std::string title() const override;
      void run() override;

    private:
      void offerFavorites(const std::vector< core::Question > &questions);

      Renderer renderer_;
      Forms forms_;
      Prompter prompter_;
      std::shared_ptr< service::QuestionService > question_service_;
      std::shared_ptr< service::FavoriteService > favorite_service_;
      size_t user_id_;
    };
  }
}

#endif
