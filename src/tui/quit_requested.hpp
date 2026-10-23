#ifndef QUIT_REQUESTED_HPP
#define QUIT_REQUESTED_HPP

#include <stdexcept>

namespace quiz
{
  namespace tui
  {
    class QuitRequested: public std::runtime_error
    {
    public:
      QuitRequested();
    };
  }
}

#endif
