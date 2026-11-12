#ifndef SMARTEXAM_LECTURE_SCREEN_H
#define SMARTEXAM_LECTURE_SCREEN_H
#include "ui/Screen.h"
#include "ui/AppContext.h"
namespace exam {
namespace ui {
class LectureScreen : public Screen {
    AppContext &context;
    Lecture draft;
    std::string attachment, search, skill;
    void view(const std::string &id);
    void edit();

  public:
    LectureScreen(const sf::Font &font, AppContext &state) : Screen(font), context(state) {}
    void refresh();
};
} // namespace ui
} // namespace exam
#endif
