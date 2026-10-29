#ifndef SMARTEXAM_LESSON_SCREEN_H
#define SMARTEXAM_LESSON_SCREEN_H
#include "ui/Screen.h"
#include "ui/AppContext.h"
#include "ui/FormulaView.h"
#include "graphics/GraphView.h"
namespace exam {
namespace ui {
class LessonScreen : public Screen {
    AppContext &context;
    int partIndex;
    bool stepMode;
    std::string attempt;
    void send(bool finalAnswer, const std::string &operation = "", const std::string &operand = "");
    void resources(int index);

  public:
    LessonScreen(const sf::Font &font, AppContext &state)
        : Screen(font), context(state), partIndex(0), stepMode(false) {}
    void refresh();
};
} // namespace ui
} // namespace exam
#endif
