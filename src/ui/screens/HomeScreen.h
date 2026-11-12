#ifndef SMARTEXAM_HOME_SCREEN_H
#define SMARTEXAM_HOME_SCREEN_H
#include "ui/Screen.h"
#include "ui/AppContext.h"
namespace exam {
namespace ui {
class HomeScreen : public Screen {
    AppContext &context;
    void settings();
    void openLesson(const std::string &id);
    std::string homeProfile, selectedLesson;
    bool showPractice = false, showCompleted = false, profilesArchived = false;
    void teacherHome();
    void manageProfiles();
    bool reviewPending = true, reviewAll = true;
    void reviewQueue(bool pendingOnly = true, bool allStudents = true);
    void reviewDetail(const ReviewItem &item);
    void reviewResource(const ReviewItem &item, int index);

  public:
    HomeScreen(const sf::Font &font, AppContext &state) : Screen(font), context(state) {}
    void refresh();
    void openReview() {
        reviewQueue();
    }
    void openProfiles() {
        manageProfiles();
    }
};
class ResultsScreen : public Screen {
    AppContext &context;
    std::string selectedAttempt;

  public:
    ResultsScreen(const sf::Font &font, AppContext &state) : Screen(font), context(state) {}
    void refresh();
};
} // namespace ui
} // namespace exam
#endif
