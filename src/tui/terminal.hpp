#ifndef TERMINAL_HPP
#define TERMINAL_HPP

#include <cstddef>
#include <iosfwd>
#include <optional>
#include <string>

namespace quiz
{
  namespace tui
  {
    enum class TextStyle
    {
      PLAIN = 0,
      TITLE = 1,
      SUCCESS = 2,
      ERROR = 3,
      MUTED = 4
    };

    class ITerminal
    {
    public:
      virtual ~ITerminal() = default;
      virtual std::optional< std::string > readLine(const std::string &prompt) = 0;
      virtual void write(const std::string &text, TextStyle style) = 0;
      virtual void clear() = 0;
      virtual size_t width() const = 0;
    };

    class StreamTerminal: public ITerminal
    {
    public:
      StreamTerminal(std::istream &input, std::ostream &output, bool use_colors);
      std::optional< std::string > readLine(const std::string &prompt) override;
      void write(const std::string &text, TextStyle style) override;
      void clear() override;
      size_t width() const override;

    private:
      std::istream &input_;
      std::ostream &output_;
      bool use_colors_;
    };
  }
}

#endif
