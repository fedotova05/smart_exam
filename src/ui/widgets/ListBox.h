#ifndef SMARTEXAM_LIST_BOX_H
#define SMARTEXAM_LIST_BOX_H
#include "Widget.h"
#include <vector>
#include <map>
#include <algorithm>
namespace exam {
namespace ui {
class ListBox : public Widget {
    std::vector<std::string> items;
    int selected, first;
    float rowHeight;
    bool checkable;
    std::map<int, int> checks;
    std::string emptyText;
    mutable sf::RenderTexture canvas;
    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;

  public:
    std::function<void(int)> onChange;
    std::function<void(int)> onActivate;
    std::function<void(int)> onToggle;
    void setCheckable(bool value) {
        checkable = value;
    }
    void setChecked(int index, bool value) {
        checks[index] = value ? 1 : 0;
    }
    void setPartial(int index) {
        checks[index] = 2;
    }
    void setRowHeight(float height) {
        rowHeight = std::max(40.f, height);
    }
    void setEmptyText(const std::string &value) {
        emptyText = value;
    }
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
