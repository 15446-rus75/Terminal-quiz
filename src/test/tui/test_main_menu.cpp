#include "test_support.hpp"
#include "tui/main_menu.hpp"
#include "tui/screen.hpp"
#include <boost/test/unit_test.hpp>

namespace
{
  class MockScreen: public quiz::tui::IScreen
  {
  public:
    MockScreen(const std::string &title): title_(title), run_count_(0)
    {
    }

    std::string title() const override
    {
      return title_;
    }

    void run() override
    {
      ++run_count_;
    }

    size_t runCount() const
    {
      return run_count_;
    }

  private:
    std::string title_;
    size_t run_count_;
  };
}

BOOST_AUTO_TEST_SUITE(main_menu_tests)

BOOST_AUTO_TEST_CASE(main_menu_exits_on_zero)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"0"});

  auto screen1 = std::make_shared< MockScreen >("Screen 1");
  auto screen2 = std::make_shared< MockScreen >("Screen 2");

  quiz::tui::MainMenu menu(terminal, {screen1, screen2});
  menu.run();

  BOOST_CHECK_EQUAL(screen1->runCount(), 0);
  BOOST_CHECK_EQUAL(screen2->runCount(), 0);
}

BOOST_AUTO_TEST_CASE(main_menu_runs_selected_screen)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"1", "0"});

  auto screen1 = std::make_shared< MockScreen >("Screen 1");
  auto screen2 = std::make_shared< MockScreen >("Screen 2");

  quiz::tui::MainMenu menu(terminal, {screen1, screen2});
  menu.run();

  BOOST_CHECK_EQUAL(screen1->runCount(), 1);
  BOOST_CHECK_EQUAL(screen2->runCount(), 0);
}

BOOST_AUTO_TEST_CASE(main_menu_runs_multiple_screens)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"1", "2", "1", "0"});

  auto screen1 = std::make_shared< MockScreen >("Screen 1");
  auto screen2 = std::make_shared< MockScreen >("Screen 2");

  quiz::tui::MainMenu menu(terminal, {screen1, screen2});
  menu.run();

  BOOST_CHECK_EQUAL(screen1->runCount(), 2);
  BOOST_CHECK_EQUAL(screen2->runCount(), 1);
}

BOOST_AUTO_TEST_CASE(main_menu_exits_on_back_command)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"/back"});

  auto screen1 = std::make_shared< MockScreen >("Screen 1");

  quiz::tui::MainMenu menu(terminal, {screen1});
  menu.run();

  BOOST_CHECK_EQUAL(screen1->runCount(), 0);
}

BOOST_AUTO_TEST_CASE(main_menu_throws_on_null_screen)
{
  auto terminal = std::make_shared< test::FakeTerminal >();

  BOOST_CHECK_THROW(quiz::tui::MainMenu(terminal, {nullptr}), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()
