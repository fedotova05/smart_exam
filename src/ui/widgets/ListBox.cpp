#include "ListBox.h"
#include <algorithm>
namespace exam {
namespace ui {
ListBox::ListBox(const sf::Font &face, float width, float height)
    : Widget(face, width, height), selected(-1), first(0), rowHeight(40), checkable(false),
      emptyText("Нет элементов") {}
void ListBox::setItems(const std::vector<std::string> &values) {
    items = values;
    selected = items.empty() ? -1 : 0;
    first = 0;
    checks.clear();
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
        sf::Vector2f point = getInverseTransform().transformPoint(mouse);
        int index = first + static_cast<int>(point.y / rowHeight);
        if (index >= static_cast<int>(items.size()))
            return true;
        select(index, !(checkable && point.x < 36));
        if (checkable && point.x < 36 && onToggle)
            onToggle(index);
        return true;
    }
    if (event.type == sf::Event::MouseWheelScrolled && contains(mouse)) {
        int rows = std::max(1, static_cast<int>(size.y / rowHeight));
        first = std::max(0, std::min(std::max(0, static_cast<int>(items.size()) - rows),
                                     first - static_cast<int>(event.mouseWheelScroll.delta * 3)));
        return true;
    }
    if (focused && event.type == sf::Event::KeyPressed) {
        if (checkable && event.key.code == sf::Keyboard::Space && selected >= 0) {
            if (onToggle)
                onToggle(selected);
            return true;
        }
        if (event.key.code == sf::Keyboard::Up) {
            select(std::max(0, selected - 1), true);
            return true;
        }
        if (event.key.code == sf::Keyboard::Down) {
            select(std::min(static_cast<int>(items.size()) - 1, selected + 1), true);
            return true;
        }
        if (event.key.code == sf::Keyboard::Enter) {
            if (selected >= 0 && onActivate)
                onActivate(selected);
            else if (selected >= 0 && onChange)
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
    prepareCanvas(canvas, size);
    canvas.clear(Theme::background);
    if (items.empty())
        drawText(canvas, label(font, wrap(font, unicode(emptyText), 16, size.x - 24, 4), 16, 12, 16,
                               Theme::muted));
    for (int i = first; i < static_cast<int>(items.size()) && (i - first + 1) * rowHeight <= size.y;
         ++i) {
        float y = (i - first) * rowHeight;
        if (i == selected)
            rectangle(canvas, sf::RenderStates(), sf::Vector2f(size.x, rowHeight), Theme::pale,
                      sf::Vector2f(0, y));
        float left = checkable ? 38.f : 12.f;
        unsigned textSize = rowHeight >= 48 ? 16 : 14;
        drawText(canvas, label(font, wrap(font, unicode(items[i]), textSize, size.x - left - 14, 2),
                               textSize, left, y + (rowHeight >= 48 ? 8 : 5)));
        if (checkable) {
            auto value = checks.find(i);
            int checked = value == checks.end() ? 0 : value->second;
            rectangle(canvas, sf::RenderStates(), sf::Vector2f(18, 18),
                      checked ? Theme::ink : Theme::paper, sf::Vector2f(10, y + 12), Theme::muted);
            if (checked == 2) {
                rectangle(canvas, sf::RenderStates(), sf::Vector2f(10, 2), Theme::white,
                          sf::Vector2f(14, y + 20));
            } else if (checked) {
                sf::VertexArray mark(sf::LineStrip, 3);
                mark[0] = sf::Vertex(sf::Vector2f(13, y + 20), Theme::white);
                mark[1] = sf::Vertex(sf::Vector2f(17, y + 24), Theme::white);
                mark[2] = sf::Vertex(sf::Vector2f(25, y + 16), Theme::white);
                canvas.draw(mark);
            }
        }
        rectangle(canvas, sf::RenderStates(), sf::Vector2f(size.x, 1), Theme::line,
                  sf::Vector2f(0, y + rowHeight - 1));
    }
    if (items.size() * rowHeight > size.y) {
        float height = size.y * size.y / (items.size() * rowHeight);
        rectangle(canvas, sf::RenderStates(), sf::Vector2f(3, height), Theme::accent,
                  sf::Vector2f(size.x - 4, size.y * first / items.size()));
    }
    canvas.display();
    sf::Sprite sprite = canvasSprite(canvas);
    drawCanvas(target, sprite, states);
    if (focused)
        rectangle(target, states, size, sf::Color::Transparent, sf::Vector2f(), Theme::accent);
}
} // namespace ui
} // namespace exam
