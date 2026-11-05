#include "main_menu.hpp"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "input_parsing.hpp"
#include "prompter.hpp"
#include "quit_requested.hpp"
#include "renderer.hpp"
#include "screen.hpp"
#include "terminal.hpp"

namespace
{
  struct ScreenTitle
  {
    std::string operator()(const std::shared_ptr< quiz::tui::IScreen > &screen) const
    {
      return screen->title();
    }
  };
}

quiz::tui::MainMenu::MainMenu(std::shared_ptr< ITerminal > terminal, std::vector< std::shared_ptr< IScreen > > screens):
  renderer_(terminal), prompter_(std::move(terminal)), screens_(std::move(screens))
{
  if (std::ranges::find(screens_, nullptr) != screens_.end())
  {
    throw std::invalid_argument("Screens must not be null");
  }
}

void quiz::tui::MainMenu::run()
{
  std::vector< std::string > titles;
  titles.reserve(screens_.size());
  std::ranges::transform(screens_, std::back_inserter(titles), ScreenTitle{});

  bool running = true;
  while (running)
  {
    try
    {
      renderer_.showTitle("Terminal Quiz");
      renderer_.showMenu(titles, "Exit");
      const std::optional< size_t > CHOICE =
        prompter_.askNumber("Choose an option", NumberRange{.min = 0, .max = screens_.size()});
      if (!CHOICE.has_value() || *CHOICE == 0)
      {
        running = false;
      }
      else
      {
        screens_[*CHOICE - 1]->run();
      }
    }
    catch (const QuitRequested &)
    {
      running = false;
    }
    catch (const std::exception &error)
    {
      renderer_.showError(std::string("Error: ") + error.what());
    }
  }
  renderer_.showInfo("Goodbye!");
}
