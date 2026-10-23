#ifndef SCREEN_HPP
#define SCREEN_HPP

#include <string>

namespace quiz
{
  namespace tui
  {
    class IScreen
    {
    public:
      virtual ~IScreen() = default;
      virtual std::string title() const = 0;
      virtual void run() = 0;
    };
  }
}

#endif
