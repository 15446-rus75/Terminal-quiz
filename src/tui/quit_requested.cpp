#include "quit_requested.hpp"

#include <stdexcept>

quiz::tui::QuitRequested::QuitRequested(): std::runtime_error("Quit requested")
{
}
