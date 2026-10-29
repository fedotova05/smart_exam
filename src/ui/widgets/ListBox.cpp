#include "ListBox.h"
#include <algorithm>
namespace exam {
namespace ui {
ListBox::ListBox(const sf::Font &face, float width, float height)
    : Widget(face, width, height), selected(-1), first(0), rowHeight(40) {
    canvas.create(static_cast<unsigned>(width), static_cast<unsigned>(height));
}
void ListBox::setItems(const std::vector<std::string> &values) {
    items = values;
    selected = items.empty() ? -1 : 0;
    first = 0;
}
void ListBox::select(int index, bool notify) {
    if (index < 0) {
        selected = -1;
        return;
    }
    if (index >= static_cast<int>(items.size()))
        return;
    selected = index;
    if (selected < first)
        first = selected;
    int rows = std::max(1, static_cast<int>(size.y / rowHeight));
    if (selected >= first + rows)
        first = selected - rows + 1;
    if (notify && onChange)
        onChange(selected);
}
bool ListBox::handle(const sf::Event &event, sf::Vector2f mouse) {
    if (!enabled || !visible)
        return false;
    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left && contains(mouse)) {
        select(first + static_cast<int>(getInverseTransform().transformPoint(mouse).y / rowHeight),
               true);
        return true;
    }
    if (event.type == sf::Event::MouseWheelScrolled && contains(mouse)) {
        int rows = std::max(1, static_cast<int>(size.y / rowHeight));
        first = std::max(0, std::min(std::max(0, static_cast<int>(items.size()) - rows),
                                     first - static_cast<int>(event.mouseWheelScroll.delta * 3)));
        return true;
    }
    if (focused && event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Up) {
            select(std::max(0, selected - 1), true);
            return true;
        }
        if (event.key.code == sf::Keyboard::Down) {
            select(std::min(static_cast<int>(items.size()) - 1, selected + 1), true);
            return true;
        }
        if (event.key.code == sf::Keyboard::Enter) {
            if (selected >= 0 && onChange)
                onChange(selected);
            return true;
        }
    }
    return false;
}
void ListBox::draw(sf::RenderTarget &target, sf::RenderStates states) const {
    if (!visible)
        return;
    states.transform *= getTransform();
    canvas.clear(Theme::background);
    if (items.empty())
        canvas.draw(label(font, unicode("Список пуст"), 16, 12, 16, Theme::muted));
    for (int i = first; i < static_cast<int>(items.size()) && (i - first) * rowHeight < size.y;
         ++i) {
        float y = (i - first) * rowHeight;
        if (i == selected)
            rectangle(canvas, sf::RenderStates(), sf::Vector2f(size.x, rowHeight), Theme::pale,
                      sf::Vector2f(0, y));
        canvas.draw(label(font, shorten(font, unicode(items[i]), 16, size.x - 26), 16, 12, y + 12));
        rectangle(canvas, sf::RenderStates(), sf::Vector2f(size.x, 1), Theme::line,
                  sf::Vector2f(0, y + rowHeight - 1));
    }
    if (items.size() * rowHeight > size.y) {
        float height = size.y * size.y / (items.size() * rowHeight);
        rectangle(canvas, sf::RenderStates(), sf::Vector2f(3, height), Theme::accent,
                  sf::Vector2f(size.x - 4, size.y * first / items.size()));
    }
    canvas.display();
    sf::Sprite sprite(canvas.getTexture());
    target.draw(sprite, states);
    if (focused)
        rectangle(target, states, size, sf::Color::Transparent, sf::Vector2f(), Theme::accent);
}
} // namespace ui
} // namespace exam
