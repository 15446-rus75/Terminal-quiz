#ifndef STATISTICS_SCREEN_HPP
#define STATISTICS_SCREEN_HPP

#include <cstddef>
#include <memory>
#include <string>

#include "../services/statistics_service.hpp"
#include "renderer.hpp"
#include "screen.hpp"
#include "terminal.hpp"

namespace quiz
{
  namespace tui
  {
    class StatisticsScreen: public IScreen
    {
    public:
      StatisticsScreen(std::shared_ptr< ITerminal > terminal,
                       std::shared_ptr< service::StatisticsService > statistics_service, size_t user_id);
      std::string title() const override;
      void run() override;

    private:
      Renderer renderer_;
      std::shared_ptr< service::StatisticsService > statistics_service_;
      size_t user_id_;
    };
  }
}

#endif
