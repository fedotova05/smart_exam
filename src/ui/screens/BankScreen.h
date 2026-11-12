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
    std::map<std::string, std::map<int, TaskPart>> typeDrafts;
    std::map<std::string, std::string> scoreDrafts;
    void chooseType(int selected = -1);
    void changeType(int kind);
    void partsEditor();
    void extraSettings();
    void readScores();
    static std::string typeLabel(PartKind kind);
    static TaskPart examplePart(PartKind kind);
    TaskPart answerDraft;
    int choiceFirst = 0;
    void answerSettings(bool reset = true);
    void selectSkills();
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
