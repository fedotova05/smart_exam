#include "Screen.h"
namespace exam {
namespace ui {
void Screen::clear() {
    widgets.clear();
    captions.clear();
    surfaces.clear();
    focus = -1;
    status.clear();
}
void Screen::heading(const std::string &value, float x, float y, unsigned size) {
    captions.push_back(label(*headingFont, unicode(value), size, x, y));
}
void Screen::note(const std::string &value, float x, float y, float width, sf::Color color) {
    captions.push_back(label(font, wrap(font, unicode(value), 14, width, 4), 14, x, y, color));
}
void Screen::surface(float x, float y, float width, float height, sf::Color color) {
    sf::RectangleShape shape(sf::Vector2f(width, height));
    shape.setPosition(x, y);
    shape.setFillColor(color);
    surfaces.push_back(shape);
}
void Screen::text(const std::string &value, float x, float y, unsigned size, float width,
                  int lines) {
    captions.push_back(label(font, wrap(font, unicode(value), size, width, lines), size, x, y));
}
Button *Screen::button(const std::string &title, float x, float y, float width,
                       const std::function<void()> &action, bool primary) {
    Button *item = add(new Button(font, title, width, primary), x, y);
    item->onClick = [this, action] { defer(action); };
    return item;
}
TextField *Screen::field(const std::string &value, float x, float y, float width,
                         std::size_t limit) {
    TextField *item = add(new TextField(font, width, limit), x, y);
    item->setText(value);
    return item;
}
ListBox *Screen::list(const std::vector<std::string> &rows, float x, float y, float width,
                      float height) {
    ListBox *item = add(new ListBox(font, width, height), x, y);
    item->setItems(rows);
    return item;
}
void Screen::ask(const std::string &message, const std::function<void()> &action) {
    dialog = message;
    confirmation = action;
}
void Screen::handle(const sf::Event &event, sf::Vector2f mouse) {
    mouse = getInverseTransform().transformPoint(mouse);
    if (!dialog.empty()) {
        bool yes = event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter;
        bool no = event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape;
        if (event.type == sf::Event::MouseButtonReleased &&
            event.mouseButton.button == sf::Mouse::Left) {
            yes = sf::FloatRect(220, 330, 180, 44).contains(mouse);
            no = sf::FloatRect(420, 330, 180, 44).contains(mouse);
        }
        if (yes) {
            pending = confirmation;
            dialog.clear();
        }
        if (no)
            dialog.clear();
        return;
    }
    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Tab) {
        if (widgets.empty())
            return;
        int next = focus;
        for (std::size_t i = 0; i < widgets.size(); ++i) {
            next = (next + (event.key.shift ? -1 : 1) + static_cast<int>(widgets.size())) %
                   widgets.size();
            if (widgets[next]->focusable() && widgets[next]->isVisible() &&
                widgets[next]->isEnabled())
                break;
        }
        if (focus >= 0)
            widgets[focus]->setFocus(false);
        focus = next;
        widgets[focus]->setFocus(true);
        return;
    }
    if (event.type == sf::Event::MouseButtonPressed) {
        if (focus >= 0)
            widgets[focus]->setFocus(false);
        focus = -1;
        for (std::size_t i = 0; i < widgets.size(); ++i)
            if (widgets[i]->isVisible() && widgets[i]->isEnabled() && widgets[i]->contains(mouse) &&
                widgets[i]->focusable())
                focus = static_cast<int>(i);
        if (focus >= 0)
            widgets[focus]->setFocus(true);
    }
    for (auto &widget : widgets)
        if (widget->isVisible() && widget->isEnabled())
            widget->handle(event, mouse);
}
void Screen::update() {
    if (!pending)
        return;
    auto action = pending;
    pending = std::function<void()>();
    try {
        action();
    } catch (const std::exception &error) {
        status = error.what();
    }
}
void Screen::draw(sf::RenderTarget &target, sf::RenderStates states) const {
    states.transform *= getTransform();
    target.draw(label(*headingFont, shorten(*headingFont, unicode(title), 32, 940), 32, 0, -57),
                states);
    for (const auto &surface : surfaces)
        target.draw(surface, states);
    for (const auto &caption : captions)
        target.draw(caption, states);
    for (const auto &widget : widgets)
        if (widget->isVisible())
            target.draw(*widget, states);
    target.draw(label(font, wrap(font, unicode(status), 16, 930, 2), 16, 0, 634, Theme::accent),
                states);
    if (!dialog.empty()) {
        rectangle(target, states, sf::Vector2f(940, 620), sf::Color(32, 56, 49, 140));
        rectangle(target, states, sf::Vector2f(560, 240), Theme::paper, sf::Vector2f(160, 160),
                  Theme::line);
        target.draw(label(font, wrap(font, unicode(dialog), 20, 500, 5), 20, 190, 190), states);
        rectangle(target, states, sf::Vector2f(180, 44), Theme::accent, sf::Vector2f(220, 330));
        rectangle(target, states, sf::Vector2f(180, 44), Theme::pale, sf::Vector2f(420, 330));
        target.draw(label(font, unicode("Да · Enter"), 18, 240, 341, Theme::white), states);
        target.draw(label(font, unicode("Отмена · Esc"), 18, 440, 341), states);
    }
}
} // namespace ui
} // namespace exam
