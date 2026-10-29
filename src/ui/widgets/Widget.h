#ifndef SMARTEXAM_WIDGET_H
#define SMARTEXAM_WIDGET_H
#include "ui/Theme.h"
#include <functional>
namespace exam {
namespace ui {
class Widget : public sf::Drawable, public sf::Transformable {
  protected:
    const sf::Font &font;
    sf::Vector2f size;
    bool focused, enabled, visible;

  public:
    Widget(const sf::Font &face, float width, float height)
        : font(face), size(width, height), focused(false), enabled(true), visible(true) {}
    virtual ~Widget() {}
    virtual bool handle(const sf::Event &event, sf::Vector2f mouse) = 0;
    virtual bool focusable() const {
        return true;
    }
    void setFocus(bool value) {
        focused = value;
    }
    bool hasFocus() const {
        return focused;
    }
    void setEnabled(bool value) {
        enabled = value;
    }
    bool isEnabled() const {
        return enabled;
    }
    void setVisible(bool value) {
        visible = value;
    }
    bool isVisible() const {
        return visible;
    }
    sf::Vector2f getSize() const {
        return size;
    }
    bool contains(sf::Vector2f mouse) const {
        sf::Vector2f local = getInverseTransform().transformPoint(mouse);
        return sf::FloatRect(0, 0, size.x, size.y).contains(local);
    }
};
} // namespace ui
} // namespace exam
#endif
