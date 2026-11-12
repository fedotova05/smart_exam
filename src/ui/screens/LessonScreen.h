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
    std::string feedback, detail, stepEquation, stepOperand;
    bool focusCanvas = false, answerSubmitted = false;
    int stepOperation = 0;
    void focusedTask();
    void focusedDetails();
    void focusedResult();
    void finishFocused();
    void send(bool finalAnswer, const std::string &operation = "", const std::string &operand = "");
    void resources(int index);
    void route();
    void supportChoice();
    void lectureChoices();
    void showLecture(const std::string &id);

  public:
    LessonScreen(const sf::Font &font, AppContext &state)
        : Screen(font), context(state), partIndex(0), stepMode(false) {}
    bool focusLayout() const {
        return focusCanvas;
    }
    void refresh();
    void handle(const sf::Event &event, sf::Vector2f mouse) override;
    void tick() override;
    sf::Clock timer;
};
} // namespace ui
} // namespace exam
#endif
