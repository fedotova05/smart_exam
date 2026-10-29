#ifndef SMARTEXAM_KNOWLEDGE_SCREEN_H
#define SMARTEXAM_KNOWLEDGE_SCREEN_H
#include "ui/Screen.h"
#include "ui/AppContext.h"
namespace exam {
namespace ui {
class KnowledgeScreen : public Screen {
    AppContext &context;
    std::string topic, selected;
    void editSkill(bool create);

  public:
    KnowledgeScreen(const sf::Font &font, AppContext &state) : Screen(font), context(state) {}
    void refresh();
};
} // namespace ui
} // namespace exam
#endif
