#include "Button.h"
namespace exam {
namespace ui {
namespace {
sf::Color blend(sf::Color from, sf::Color to, float amount) {
    return sf::Color(static_cast<sf::Uint8>(from.r + (to.r - from.r) * amount),
                     static_cast<sf::Uint8>(from.g + (to.g - from.g) * amount),
                     static_cast<sf::Uint8>(from.b + (to.b - from.b) * amount));
}
} // namespace
Button::Button(const sf::Font &face, const std::string &text, float width, bool filled)
    : Widget(face, width, 40), caption(unicode(text)), pressed(false), hovered(false),
      primary(filled), navigationStyle(false), selected(false) {}
bool Button::handle(const sf::Event &event, sf::Vector2f mouse) {
    hovered = contains(mouse);
    if (!enabled || !visible) {
        pressed = false;
        return false;
    }
    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left) {
        pressed = hovered;
        return pressed;
    }
    if (event.type == sf::Event::MouseButtonReleased &&
        event.mouseButton.button == sf::Mouse::Left) {
        bool activate = pressed && hovered;
        pressed = false;
        if (activate && onClick)
            onClick();
        return activate;
    }
    if (event.type == sf::Event::KeyPressed && focused &&
        (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Space)) {
        if (onClick)
            onClick();
        return true;
    }
    return false;
}
void Button::draw(sf::RenderTarget &target, sf::RenderStates states) const {
    if (!visible)
        return;
    states.transform *= getTransform();
    float elapsed = std::min(.05f, hoverClock.restart().asSeconds());
    float goal = enabled && (hovered || pressed) ? 1.f : 0.f;
    hoverAmount = !motionEnabled()     ? goal
                  : goal > hoverAmount ? std::min(goal, hoverAmount + elapsed / .12f)
                                       : std::max(goal, hoverAmount - elapsed / .12f);
    if (navigationStyle) {
        if (selected || hoverAmount > 0)
            rectangle(target, states, size,
                      blend(Theme::paper, Theme::pale, selected ? 1.f : hoverAmount));
        if (selected)
            rectangle(target, states, sf::Vector2f(size.x, 2), Theme::ink,
                      sf::Vector2f(0, size.y - 2));
        if (focused)
            rectangle(target, states, size, sf::Color::Transparent, sf::Vector2f(), Theme::accent);
        drawText(target,
                 label(font, shorten(font, caption, 15, size.x - 32), 15, 18, 12,
                       selected ? Theme::ink : Theme::muted),
                 states);
        return;
    }
    sf::Color fill = primary ? Theme::ink : Theme::paper;
    fill = blend(fill, primary ? sf::Color(52, 79, 66) : Theme::pale, hoverAmount);
    if (!enabled)
        fill = Theme::line;
    rectangle(target, states, size, fill, sf::Vector2f(), primary ? fill : Theme::line);
    if (focused)
        rectangle(target, states, size, sf::Color::Transparent, sf::Vector2f(), Theme::accent);
    sf::String text = shorten(font, caption, 15, size.x - 20);
    sf::Text captionText =
        label(font, text, 15, 0, 0, primary && enabled ? Theme::white : Theme::ink);
    captionText.setPosition(
        (size.x - captionText.getLocalBounds().width * captionText.getScale().x) / 2,
        11 - captionText.getLocalBounds().top * captionText.getScale().y);
    drawText(target, captionText, states);
}
Toggle::Toggle(const sf::Font &face, const std::string &text, float width)
    : Button(face, text, width), value(false), title(text) {
    onClick = [this]() {
        setValue(!value);
        if (onChange)
            onChange(value);
    };
    setValue(false);
}
void Toggle::setValue(bool checked) {
    value = checked;
    setText(std::string(value ? "[x] " : "[ ] ") + title);
}
void Toggle::draw(sf::RenderTarget &target, sf::RenderStates states) const {
    states.transform *= getTransform();
    if (hovered)
        rectangle(target, states, size, Theme::pale);
    rectangle(target, states, sf::Vector2f(15, 15), value ? Theme::ink : Theme::background,
              sf::Vector2f(8, 13), focused ? Theme::accent : Theme::muted);
    if (value) {
        sf::Vertex tick[] = {sf::Vertex(sf::Vector2f(11, 20), Theme::white),
                             sf::Vertex(sf::Vector2f(15, 24), Theme::white),
                             sf::Vertex(sf::Vector2f(21, 16), Theme::white)};
        target.draw(tick, 3, sf::LineStrip, states);
    }
    drawText(target, label(font, shorten(font, unicode(title), 15, size.x - 40), 15, 34, 12),
             states);
}
} // namespace ui
} // namespace exam
