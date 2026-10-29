#ifndef SMARTEXAM_HOME_SCREEN_H
#define SMARTEXAM_HOME_SCREEN_H
#include "ui/Screen.h"
#include "ui/AppContext.h"
namespace exam {
namespace ui {
class HomeScreen : public Screen {
    AppContext &context;
    std::vector<std::string> fixedTasks;
    void settings();
    void startShortLesson();

  public:
    HomeScreen(const sf::Font &font, AppContext &state) : Screen(font), context(state) {}
    void refresh();
};
class ResultsScreen : public Screen {
    AppContext &context;

  public:
    ResultsScreen(const sf::Font &font, AppContext &state) : Screen(font), context(state) {}
    void refresh();
};
} // namespace ui
} // namespace exam
#endif
