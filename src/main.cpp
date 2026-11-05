#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>

#include "app/application.hpp"
#include "app/config.hpp"
#include "tui/terminal.hpp"

int main()
{
  try
  {
    const quiz::app::AppConfig CONFIG = quiz::app::loadConfigFromEnvironment();
    auto terminal = std::make_shared< quiz::tui::StreamTerminal >(std::cin, std::cout, CONFIG.use_colors);
    quiz::app::Application application(CONFIG, terminal);
    return application.run();
  }
  catch (const std::exception &error)
  {
    std::cerr << "Fatal error: " << error.what() << std::endl;
    return EXIT_FAILURE;
  }
}
