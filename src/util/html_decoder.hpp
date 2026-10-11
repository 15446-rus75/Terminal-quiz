#ifndef HTML_DECODER_HPP
#define HTML_DECODER_HPP

#include <string>

namespace quiz
{
  namespace util
  {
    class HtmlDecoder
    {
    public:
      static std::string decode(const std::string &encoded);

    private:
      static std::string decodeEntity(const std::string &entity);
      static std::string decodeNumericEntity(const std::string &entity);
    };
  }
}

#endif
