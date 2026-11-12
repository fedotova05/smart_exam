#include "TextArea.h"
#include <SFML/Window/Clipboard.hpp>
#include <algorithm>
#include <cmath>
namespace exam {
namespace ui {
std::vector<TextArea::Line> TextArea::lines() const {
    std::vector<Line> result;
    std::size_t start = 0;
    float width = 0;
    for (std::size_t i = 0; i < value.getSize(); ++i) {
        if (value[i] == '\n') {
            result.push_back({start, i});
            start = i + 1;
            width = 0;
            continue;
        }
        float next = font.getGlyph(value[i], 18, false).advance;
        if (width + next > size.x - 30 && i > start) {
            result.push_back({start, i});
            start = i;
            width = 0;
        }
        width += next;
    }
    result.push_back({start, value.getSize()});
    return result;
}
void TextArea::insert(const sf::String &input) {
    sf::String clean;
    for (auto c : input)
        if (c == '\n' || (c >= 32 && c != 127))
            clean += c;
    if (all) {
        value.clear();
        cursor = 0;
        all = false;
    }
    if (value.getSize() >= 15000)
        clean.clear();
    else if (value.getSize() + clean.getSize() > 15000)
        clean.erase(15000 - value.getSize());
    value.insert(cursor, clean);
    cursor += clean.getSize();
    if (onChange)
        onChange();
}
bool TextArea::handle(const sf::Event &e, sf::Vector2f mouse) {
    if (!visible || !enabled)
        return false;
    auto rows = lines();
    if (e.type == sf::Event::MouseWheelScrolled && contains(mouse)) {
        scroll = std::max(0.f, std::min(std::max(0.f, float(rows.size()) * 26 - size.y + 20),
                                        scroll - e.mouseWheelScroll.delta * 52));
        return true;
    }
    if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left &&
        contains(mouse)) {
        auto point = getInverseTransform().transformPoint(mouse);
        int row = std::max(0, std::min(int(rows.size()) - 1, int((point.y - 8 + scroll) / 26)));
        auto line = rows[row];
        cursor = line.start;
        float x = 10;
        while (cursor < line.end &&
               x + font.getGlyph(value[cursor], 18, false).advance / 2 < point.x)
            x += font.getGlyph(value[cursor++], 18, false).advance;
        all = false;
        return true;
    }
    if (!focused)
        return false;
    bool changed = false, moved = false;
    if (e.type == sf::Event::TextEntered && e.text.unicode >= 32 && e.text.unicode != 127 &&
        !sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) &&
        !sf::Keyboard::isKeyPressed(sf::Keyboard::RControl)) {
        insert(sf::String(e.text.unicode));
        moved = true;
    } else if (e.type == sf::Event::KeyPressed) {
        auto key = e.key.code;
        if (e.key.control && key == sf::Keyboard::A) {
            all = true;
            return true;
        }
        if (e.key.control && (key == sf::Keyboard::C || key == sf::Keyboard::X)) {
            if (all)
                sf::Clipboard::setString(value);
            if (key == sf::Keyboard::X && all) {
                value.clear();
                cursor = 0;
                all = false;
                changed = true;
            }
        } else if (e.key.control && key == sf::Keyboard::V) {
            insert(sf::Clipboard::getString());
            moved = true;
        } else if (key == sf::Keyboard::Enter) {
            insert("\n");
            moved = true;
        } else if (key == sf::Keyboard::BackSpace || key == sf::Keyboard::Delete) {
            if (all) {
                value.clear();
                cursor = 0;
                all = false;
            } else if (key == sf::Keyboard::BackSpace && cursor)
                value.erase(--cursor);
            else if (key == sf::Keyboard::Delete && cursor < value.getSize())
                value.erase(cursor);
            changed = true;
        } else {
            int row = 0;
            for (int i = 0; i < int(rows.size()); ++i)
                if (rows[i].start <= cursor)
                    row = i;
            auto column = cursor - rows[row].start;
            if (key == sf::Keyboard::Left && cursor)
                --cursor;
            else if (key == sf::Keyboard::Right && cursor < value.getSize())
                ++cursor;
            else if (key == sf::Keyboard::Home)
                cursor = e.key.control ? 0 : rows[row].start;
            else if (key == sf::Keyboard::End)
                cursor = e.key.control ? value.getSize() : rows[row].end;
            else if (key == sf::Keyboard::Up && row > 0)
                cursor = std::min(rows[row - 1].end, rows[row - 1].start + column);
            else if (key == sf::Keyboard::Down && row + 1 < int(rows.size()))
                cursor = std::min(rows[row + 1].end, rows[row + 1].start + column);
            else
                return false;
            all = false;
            moved = true;
        }
    } else
        return false;
    if (changed && onChange)
        onChange();
    if (changed || moved) {
        rows = lines();
        int row = 0;
        for (int i = 0; i < int(rows.size()); ++i)
            if (rows[i].start <= cursor)
                row = i;
        if (row * 26 < scroll)
            scroll = row * 26;
        if (row * 26 > scroll + size.y - 44)
            scroll = row * 26 - size.y + 44;
        scroll = std::max(0.f, scroll);
    }
    return true;
}
void TextArea::draw(sf::RenderTarget &target, sf::RenderStates states) const {
    states.transform *= getTransform();
    rectangle(target, states, size, Theme::white, {}, focused ? Theme::accent : Theme::line);
    prepareCanvas(canvas, sf::Vector2f(size.x - 16, size.y - 16));
    canvas.clear(all && focused ? sf::Color(217, 227, 209) : Theme::white);
    auto rows = lines();
    int active = 0;
    for (int i = 0; i < int(rows.size()); ++i)
        if (rows[i].start <= cursor)
            active = i;
    for (std::size_t i = 0; i < rows.size(); ++i) {
        float y = i * 26 - scroll;
        if (y < -26 || y > size.y)
            continue;
        auto t = label(font, value.substring(rows[i].start, rows[i].end - rows[i].start), 18, 2, y);
        drawText(canvas, t);
        if (focused && int(i) == active) {
            auto caret = t.findCharacterPos(cursor - rows[i].start);
            rectangle(canvas, {}, sf::Vector2f(1, 23), Theme::ink, caret);
        }
    }
    canvas.display();
    auto sprite = canvasSprite(canvas);
    sprite.setPosition(8, 8);
    drawCanvas(target, sprite, states);
}
} // namespace ui
} // namespace exam
