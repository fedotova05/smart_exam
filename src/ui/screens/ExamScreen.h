#ifndef SMARTEXAM_EXAM_SCREEN_H
#define SMARTEXAM_EXAM_SCREEN_H
#include "ui/Screen.h"
#include "ui/AppContext.h"
#include "application/jobs/JobQueue.h"
#include "application/exams/ExamService.h"
namespace exam {
namespace ui {
class ExamScreen : public Screen {
    AppContext &context;
    int number = 1, category = 135;
    std::string count = "5", student;
    bool online = true;
    std::unique_ptr<JobQueue> jobs;
    std::shared_ptr<ExamLoadResult> output;
    unsigned screen = 1, request = 0;
    void load();
    void importFile();
    void result();

  public:
    ExamScreen(const sf::Font &font, AppContext &state) : Screen(font), context(state) {}
    void refresh();
    void tick() override;
    void leave() override;
};
} // namespace ui
} // namespace exam
#endif
