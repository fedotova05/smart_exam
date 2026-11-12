#ifndef SMARTEXAM_PROGRESS_SCREEN_H
#define SMARTEXAM_PROGRESS_SCREEN_H
#include "ui/Screen.h"
#include "ui/AppContext.h"
namespace exam {
namespace ui {
class ProgressScreen : public Screen {
    AppContext &context;
    std::string topic;
    int first;
    bool detailed = false;
    void overview();
    void details();
    void tabs();

  public:
    ProgressScreen(const sf::Font &font, AppContext &state)
        : Screen(font), context(state), first(0) {}
    void refresh();
    void explain();
};
} // namespace ui
} // namespace exam
#endif
