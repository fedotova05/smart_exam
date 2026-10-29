#ifndef SMARTEXAM_SKILL_MAP_H
#define SMARTEXAM_SKILL_MAP_H
#include "widgets/Widget.h"
#include "storage/Store.h"
namespace exam {
namespace ui {
class SkillMap : public Widget {
    std::vector<SkillInfo> nodes;
    std::map<std::string, std::vector<std::string>> edges;
    StudentSnapshot snapshot;
    int dragged;
    float scroll;
    sf::Vector2f offset;
    mutable sf::RenderTexture canvas;
    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;

  public:
    std::function<void(const SkillInfo &)> onSelect, onMove;
    SkillMap(const sf::Font &font, const std::vector<SkillInfo> &skills,
             const std::map<std::string, std::vector<std::string>> &graph,
             const StudentSnapshot &state);
    bool handle(const sf::Event &event, sf::Vector2f mouse) override;
};
} // namespace ui
} // namespace exam
#endif
