#ifndef SMARTEXAM_LESSON_SETUP_SCREEN_H
#define SMARTEXAM_LESSON_SETUP_SCREEN_H
#include "ui/Screen.h"
#include "ui/AppContext.h"
namespace exam {
namespace ui {
class LessonSetupScreen : public Screen {
    AppContext &context;
    std::vector<TaskVersion> selectedTasks;
    std::string profile, skill, query, selectedTask, amount;
    int source, family, basketIndex;
    unsigned seed;
    void bank();
    void generator();
    void adaptive();
    void basket();
    void review();
    void start(bool replace = false);
    int taskCount() const;

  public:
    LessonSetupScreen(const sf::Font &font, AppContext &state);
    void refresh();
};
} // namespace ui
} // namespace exam
#endif
