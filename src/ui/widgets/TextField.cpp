#include "TextField.h"
#include <SFML/Window/Clipboard.hpp>
#include <algorithm>
namespace exam {
namespace ui {
TextField::TextField(const sf::Font &face, float width, std::size_t limit)
    : Widget(face, width, 40), cursor(0), maximum(limit), selectedAll(false), scroll(0) {
    canvas.create(static_cast<unsigned>(width - 16), 32);
}
void TextField::setText(const std::string &text) {
    value = unicode(text);
    if (value.getSize() > maximum)
        value.erase(maximum);
    cursor = value.getSize();
    selectedAll = false;
    scroll = 0;
}
void TextField::changed() {
    if (onChange)
        onChange();
}
void TextField::insert(const std::string &text) {
    sf::String insertion = unicode(text), clean;
    for (sf::Uint32 c : insertion)
        if (c >= 32 && c != 127)
            clean += c;
    if (selectedAll) {
        value.clear();
        cursor = 0;
        selectedAll = false;
    }
    if (clean.getSize() + value.getSize() > maximum)
        clean.erase(maximum - value.getSize());
    value.insert(cursor, clean);
    cursor += clean.getSize();
    changed();
}
bool TextField::handle(const sf::Event &event, sf::Vector2f mouse) {
    if (!enabled || !visible)
        return false;
    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left && contains(mouse)) {
        float x = getInverseTransform().transformPoint(mouse).x - 8 + scroll;
        sf::Text text(value, font, 18);
        cursor = 0;
        while (cursor < value.getSize() && text.findCharacterPos(cursor + 1).x < x)
            ++cursor;
        selectedAll = false;
        return true;
    }
    if (!focused)
        return false;
    if (event.type == sf::Event::TextEntered && event.text.unicode >= 32 &&
        event.text.unicode != 127 && !sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) &&
        !sf::Keyboard::isKeyPressed(sf::Keyboard::RControl)) {
        insert(utf8(sf::String(event.text.unicode)));
        return true;
    }
    if (event.type != sf::Event::KeyPressed)
        return false;
    if (event.key.control && event.key.code == sf::Keyboard::A) {
        selectedAll = true;
        return true;
    }
    if (event.key.control && event.key.code == sf::Keyboard::C) {
        if (selectedAll)
            sf::Clipboard::setString(value);
        return true;
    }
    if (event.key.control && event.key.code == sf::Keyboard::V) {
        insert(utf8(sf::Clipboard::getString()));
        return true;
    }
    if (event.key.code == sf::Keyboard::Left) {
        if (cursor > 0)
            --cursor;
        selectedAll = false;
        return true;
    }
    if (event.key.code == sf::Keyboard::Right) {
        if (cursor < value.getSize())
            ++cursor;
        selectedAll = false;
        return true;
    }
    if (event.key.code == sf::Keyboard::Home) {
        cursor = 0;
        selectedAll = false;
        return true;
    }
    if (event.key.code == sf::Keyboard::End) {
        cursor = value.getSize();
        selectedAll = false;
        return true;
    }
    if (event.key.code == sf::Keyboard::BackSpace || event.key.code == sf::Keyboard::Delete) {
        if (selectedAll) {
            value.clear();
            cursor = 0;
            selectedAll = false;
        } else if (event.key.code == sf::Keyboard::BackSpace && cursor > 0) {
            value.erase(--cursor);
        } else if (event.key.code == sf::Keyboard::Delete && cursor < value.getSize())
            value.erase(cursor);
        changed();
        return true;
    }
    if (event.key.code == sf::Keyboard::Enter) {
        if (onEnter)
            onEnter();
        return true;
    }
    return false;
}
void TextField::draw(sf::RenderTarget &target, sf::RenderStates states) const {
    if (!visible)
        return;
    states.transform *= getTransform();
    rectangle(target, states, size, enabled ? Theme::white : Theme::pale, sf::Vector2f(),
              focused ? Theme::accent : Theme::line);
    canvas.clear(enabled ? Theme::white : Theme::pale);
    sf::Text text = label(font, value.isEmpty() ? placeholder : value, 18, 0, 7,
                          value.isEmpty() ? Theme::muted : Theme::ink);
    sf::Text measure(value, font, 18);
    float caret = measure.findCharacterPos(cursor).x;
    if (caret - scroll > size.x - 24)
        scroll = caret - size.x + 24;
    if (caret - scroll < 0)
        scroll = caret;
    scroll = std::max(0.f, scroll);
    if (selectedAll && focused)
        rectangle(canvas, sf::RenderStates(),
                  sf::Vector2f(std::max(1.f, measure.getLocalBounds().width), 28),
                  sf::Color(217, 227, 209), sf::Vector2f(-scroll, 2));
    text.move(-scroll, 0);
    canvas.draw(text);
    if (focused)
        rectangle(canvas, sf::RenderStates(), sf::Vector2f(1, 24), Theme::ink,
                  sf::Vector2f(caret - scroll, 3));
    canvas.display();
    sf::Sprite sprite(canvas.getTexture());
    sprite.setPosition(8, 4);
    target.draw(sprite, states);
}
} // namespace ui
} // namespace exam
