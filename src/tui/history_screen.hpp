#ifndef HISTORY_SCREEN_HPP
#define HISTORY_SCREEN_HPP

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "../core/attempt.hpp"
#include "../services/history_service.hpp"
#include "prompter.hpp"
#include "renderer.hpp"
#include "screen.hpp"
#include "terminal.hpp"

namespace quiz
{
  namespace tui
  {
    class HistoryScreen: public IScreen
    {
    public:
      HistoryScreen(std::shared_ptr< ITerminal > terminal, std::shared_ptr< service::HistoryService > history_service,
                    size_t user_id);
      std::string title() const override;
      void run() override;

    private:
      void showDetails(const std::vector< core::Attempt > &attempts);

      Renderer renderer_;
      Prompter prompter_;
      std::shared_ptr< service::HistoryService > history_service_;
      size_t user_id_;
    };
  }
}

#endif
