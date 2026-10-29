#ifndef SMARTEXAM_BANK_SCREEN_H
#define SMARTEXAM_BANK_SCREEN_H
#include "ui/Screen.h"
#include "ui/AppContext.h"
namespace exam {
namespace ui {
class BankScreen : public Screen {
    AppContext &context;
    TaskVersion draft;
    bool editing;
    int partIndex;
    std::string query;
    void editor();
    void preview();
    void createTask(bool combined);

  public:
    BankScreen(const sf::Font &font, AppContext &state)
        : Screen(font), context(state), editing(false), partIndex(0) {}
    void refresh();
};
} // namespace ui
} // namespace exam
#endif
