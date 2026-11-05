#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <memory>

#include "../tui/terminal.hpp"
#include "config.hpp"

namespace quiz
{
  namespace app
  {
    class Application
    {
    public:
      Application(const AppConfig &config, std::shared_ptr< tui::ITerminal > terminal);
      int run();

    private:
      AppConfig config_;
      std::shared_ptr< tui::ITerminal > terminal_;
    };
  }
}

#endif
