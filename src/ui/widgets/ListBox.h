#ifndef SMARTEXAM_LIST_BOX_H
#define SMARTEXAM_LIST_BOX_H
#include "Widget.h"
#include <vector>
namespace exam {
namespace ui {
class ListBox : public Widget {
    std::vector<std::string> items;
    int selected, first;
    float rowHeight;
    mutable sf::RenderTexture canvas;
    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;

  public:
    std::function<void(int)> onChange;
    ListBox(const sf::Font &font, float width, float height);
    void setItems(const std::vector<std::string> &values);
    void select(int index, bool notify = false);
    int selection() const {
        return selected;
    }
    bool handle(const sf::Event &event, sf::Vector2f mouse) override;
};
} // namespace ui
} // namespace exam
#endif
