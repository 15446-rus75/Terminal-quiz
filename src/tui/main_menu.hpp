#ifndef MAIN_MENU_HPP
#define MAIN_MENU_HPP

#include <memory>
#include <vector>

#include "prompter.hpp"
#include "renderer.hpp"
#include "screen.hpp"
#include "terminal.hpp"

namespace quiz
{
  namespace tui
  {
    class MainMenu
    {
    public:
      MainMenu(std::shared_ptr< ITerminal > terminal, std::vector< std::shared_ptr< IScreen > > screens);
      void run();

    private:
      Renderer renderer_;
      Prompter prompter_;
      std::vector< std::shared_ptr< IScreen > > screens_;
    };
  }
}

#endif
