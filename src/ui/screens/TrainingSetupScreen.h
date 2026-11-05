#ifndef SMARTEXAM_TRAINING_SETUP_SCREEN_H
#define SMARTEXAM_TRAINING_SETUP_SCREEN_H
#include "ui/Screen.h"
#include "ui/AppContext.h"
#include <set>
namespace exam { namespace ui {
class TrainingSetupScreen : public Screen {
    AppContext &context;
    std::string profile, topic, count;
    std::set<std::string> selected;
    bool training;
  public:
    TrainingSetupScreen(const sf::Font &font, AppContext &state)
        : Screen(font), context(state), count("5"), training(true) {}
    void refresh();
};
} }
#endif
