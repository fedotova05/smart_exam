#include "Button.h"
namespace exam {
namespace ui {
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
    if (navigationStyle) {
        if (selected || hovered)
            rectangle(target, states, size, Theme::navigationActive);
        if (selected)
            rectangle(target, states, sf::Vector2f(2, size.y), Theme::accent);
        target.draw(label(font, shorten(font, caption, 15, size.x - 32), 15, 18, 12,
                          selected ? Theme::white : sf::Color(183, 199, 187)),
                    states);
        return;
    }
    sf::Color fill = primary ? Theme::ink : Theme::paper;
    if (hovered || pressed)
        fill = primary ? sf::Color(52, 79, 66) : Theme::pale;
    if (!enabled)
        fill = Theme::line;
    if (primary || hovered || pressed || !enabled)
        rectangle(target, states, size, fill);
    if (focused)
        rectangle(target, states, size, sf::Color::Transparent, sf::Vector2f(), Theme::accent);
    sf::String text = shorten(font, caption, 15, size.x - 20);
    sf::Text drawText = label(font, text, 15, 0, 0, primary && enabled ? Theme::white : Theme::ink);
    drawText.setPosition((size.x - drawText.getLocalBounds().width) / 2,
                         11 - drawText.getLocalBounds().top);
    target.draw(drawText, states);
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
    target.draw(label(font, shorten(font, unicode(title), 15, size.x - 40), 15, 34, 12), states);
}
} // namespace ui
} // namespace exam
