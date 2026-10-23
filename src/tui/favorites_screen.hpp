#ifndef FAVORITES_SCREEN_HPP
#define FAVORITES_SCREEN_HPP

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "../core/question.hpp"
#include "../services/favorite_service.hpp"
#include "prompter.hpp"
#include "quiz_runner.hpp"
#include "renderer.hpp"
#include "screen.hpp"
#include "terminal.hpp"

namespace quiz
{
  namespace tui
  {
    class FavoritesScreen: public IScreen
    {
    public:
      FavoritesScreen(std::shared_ptr< ITerminal > terminal,
                      std::shared_ptr< service::FavoriteService > favorite_service,
                      std::shared_ptr< QuizRunner > runner, size_t user_id);
      std::string title() const override;
      void run() override;

    private:
      void removeFavorites(const std::vector< core::Question > &questions);
      void playFavorites(const std::vector< core::Question > &questions);

      Renderer renderer_;
      Prompter prompter_;
      std::shared_ptr< service::FavoriteService > favorite_service_;
      std::shared_ptr< QuizRunner > runner_;
      size_t user_id_;
    };
  }
}

#endif
