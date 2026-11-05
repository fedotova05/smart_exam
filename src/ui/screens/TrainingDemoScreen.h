#ifndef SMARTEXAM_TRAINING_DEMO_SCREEN_H
#define SMARTEXAM_TRAINING_DEMO_SCREEN_H
#include "ui/Screen.h"
#include "ui/AppContext.h"
namespace exam { namespace ui {
class TrainingDemoScreen : public Screen {
    AppContext &context;
    int selected;
  public:
    TrainingDemoScreen(const sf::Font &font, AppContext &state)
        : Screen(font), context(state), selected(0) {}
    void refresh();
};
} }
#endif
