#ifndef SMARTEXAM_WELCOME_SCREEN_H
#define SMARTEXAM_WELCOME_SCREEN_H
#include "ui/Screen.h"
#include "ui/AppContext.h"
namespace exam {
namespace ui {
class WelcomeScreen : public Screen {
    AppContext &context;
    void profiles();
    void teacherLogin();
    void enter(const std::string &id, const std::string &name);

  public:
    WelcomeScreen(const sf::Font &font, AppContext &state) : Screen(font), context(state) {}
    void refresh();
};
} // namespace ui
} // namespace exam
#endif
