#include <array>
#include <string>
#include <utility>

#include "util/html_decoder.hpp"
#include <boost/test/unit_test.hpp>

using quiz::util::HtmlDecoder;

BOOST_AUTO_TEST_SUITE(html_decoder)

BOOST_AUTO_TEST_CASE(plain_text_and_empty_input)
{
  BOOST_TEST(HtmlDecoder::decode("").empty());
  BOOST_TEST(HtmlDecoder::decode("plain text 123") == "plain text 123");
  BOOST_TEST(HtmlDecoder::decode("UTF-8: \xD0\x96") == "UTF-8: \xD0\x96");
}

BOOST_AUTO_TEST_CASE(named_entities_and_case_sensitivity)
{
  BOOST_TEST(HtmlDecoder::decode("&quot;A &amp; B&quot; &lt;x&gt; &apos;y&apos;") == "\"A & B\" <x> 'y'");
  BOOST_TEST(HtmlDecoder::decode("&eacute;&Eacute;&pi;&euro;") == "\xC3\xA9\xC3\x89\xCF\x80\xE2\x82\xAC");
  BOOST_TEST(HtmlDecoder::decode("&nbsp;&copy;&reg;&mdash;") == "\xC2\xA0\xC2\xA9\xC2\xAE\xE2\x80\x94");
  BOOST_TEST(HtmlDecoder::decode("&AMP;") == "&AMP;");
}

BOOST_AUTO_TEST_CASE(decimal_and_hexadecimal_references)
{
  BOOST_TEST(HtmlDecoder::decode("&#65;&#x41;&#X41;") == "AAA");
  BOOST_TEST(HtmlDecoder::decode("&#1046;&#x416;") == "\xD0\x96\xD0\x96");
  BOOST_TEST(HtmlDecoder::decode("&#128512;&#x1F600;") == "\xF0\x9F\x98\x80\xF0\x9F\x98\x80");
  BOOST_TEST(HtmlDecoder::decode("&#00065;") == "A");
}

BOOST_AUTO_TEST_CASE(utf8_boundaries)
{
  const std::array< std::pair< std::string, std::string >, 8 > examples = {{
    {"&#x7F;", "\x7F"},
    {"&#x80;", "\xC2\x80"},
    {"&#x7FF;", "\xDF\xBF"},
    {"&#x800;", "\xE0\xA0\x80"},
    {"&#xD7FF;", "\xED\x9F\xBF"},
    {"&#xE000;", "\xEE\x80\x80"},
    {"&#x10000;", "\xF0\x90\x80\x80"},
    {"&#x10FFFF;", "\xF4\x8F\xBF\xBF"},
  }};
  for (const auto &example : examples)
  {
    BOOST_TEST_CONTEXT(example.first)
    {
      BOOST_TEST(HtmlDecoder::decode(example.first) == example.second);
    }
  }
}

BOOST_AUTO_TEST_CASE(invalid_and_unknown_entities_are_preserved)
{
  const std::array< std::string, 17 > examples = {"&unknown;",
                                                  "&;",
                                                  "&amp",
                                                  "&",
                                                  "&#;",
                                                  "&#x;",
                                                  "&#-1;",
                                                  "&#+1;",
                                                  "&# 65;",
                                                  "&#65x;",
                                                  "&#xGG;",
                                                  "&#0;",
                                                  "&#xD800;",
                                                  "&#xDFFF;",
                                                  "&#x110000;",
                                                  "&#4294967296;",
                                                  "&#999999999999999999999;"};
  for (const auto &example : examples)
  {
    BOOST_TEST_CONTEXT(example)
    {
      BOOST_TEST(HtmlDecoder::decode(example) == example);
    }
  }
}

BOOST_AUTO_TEST_CASE(decodes_once_and_recovers_after_broken_entity)
{
  BOOST_TEST(HtmlDecoder::decode("&amp;lt;") == "&lt;");
  BOOST_TEST(HtmlDecoder::decode("&broken &amp; tail") == "&broken & tail");
  BOOST_TEST(HtmlDecoder::decode("&&amp;&lt;&unknown;&gt;") == "&&<&unknown;>");
}

BOOST_AUTO_TEST_CASE(long_input_and_embedded_null_are_preserved)
{
  const std::string ampersands(100000, '&');
  BOOST_TEST(HtmlDecoder::decode(ampersands + "&amp;") == ampersands + "&");
  const std::string embedded_null("A\0&amp;B", 8);
  BOOST_TEST(HtmlDecoder::decode(embedded_null) == std::string("A\0&B", 4));
}

BOOST_AUTO_TEST_SUITE_END()
