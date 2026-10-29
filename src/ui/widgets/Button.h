#ifndef SMARTEXAM_BUTTON_H
#define SMARTEXAM_BUTTON_H
#include "Widget.h"
namespace exam {
namespace ui {
class Button : public Widget {
  protected:
    sf::String caption;
    bool pressed, hovered, primary;
    bool navigationStyle, selected;
    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;

  public:
    std::function<void()> onClick;
    Button(const sf::Font &font, const std::string &text, float width = 160, bool filled = false);
    void setText(const std::string &text) {
        caption = unicode(text);
    }
    void setNavigation(bool value) {
        navigationStyle = value;
    }
    void setSelected(bool value) {
        selected = value;
    }
    bool handle(const sf::Event &event, sf::Vector2f mouse) override;
};
class Toggle : public Button {
    bool value;
    std::string title;
    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;

  public:
    std::function<void(bool)> onChange;
    Toggle(const sf::Font &font, const std::string &text, float width = 200);
    void setValue(bool checked);
    bool getValue() const {
        return value;
    }
};
} // namespace ui
} // namespace exam
#endif
