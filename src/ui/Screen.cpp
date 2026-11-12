#include "Screen.h"
namespace exam {
namespace ui {
void Screen::clear() {
    widgets.clear();
    captions.clear();
    surfaces.clear();
    focus = -1;
    status.clear();
    clockText.clear();
    contentWidth = 940;
    setStatusArea(0, 628, 930);
}
void Screen::clearFocus() {
    if (focus >= 0 && focus < static_cast<int>(widgets.size()))
        widgets[focus]->setFocus(false);
    focus = -1;
}
void Screen::focusWidget(Widget *widget) {
    clearFocus();
    for (std::size_t i = 0; i < widgets.size(); ++i)
        if (widgets[i].get() == widget && widget->focusable() && widget->isEnabled() &&
            widget->isVisible()) {
            focus = static_cast<int>(i);
            widget->setFocus(true);
            break;
        }
}
bool Screen::moveFocus(bool backward, bool wrap) {
    if (widgets.empty())
        return false;
    int next = focus < 0 ? (backward ? static_cast<int>(widgets.size()) : -1) : focus;
    for (std::size_t i = 0; i < widgets.size(); ++i) {
        next += backward ? -1 : 1;
        if (next < 0 || next >= static_cast<int>(widgets.size())) {
            if (!wrap) {
                clearFocus();
                return false;
            }
            next = backward ? static_cast<int>(widgets.size()) - 1 : 0;
        }
        if (widgets[next]->focusable() && widgets[next]->isVisible() &&
            widgets[next]->isEnabled()) {
            focusWidget(widgets[next].get());
            return true;
        }
    }
    return false;
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
void Screen::progressBar(float value, float x, float y, float width) {
    value = std::max(0.f, std::min(1.f, value));
    surface(x, y, width, 8, Theme::line);
    surface(x, y, width * value, 8, Theme::ink);
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
            float left = (contentWidth - 560) / 2;
            yes = sf::FloatRect(left + 60, 330, 180, 44).contains(mouse);
            no = sf::FloatRect(left + 260, 330, 180, 44).contains(mouse);
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
        moveFocus(event.key.shift);
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
    target.draw(
        label(*headingFont, shorten(*headingFont, unicode(title), 32, contentWidth), 32, 0, -57),
        states);
    for (const auto &surface : surfaces)
        target.draw(surface, states);
    for (const auto &caption : captions)
        target.draw(caption, states);
    for (const auto &widget : widgets)
        if (widget->isVisible())
            target.draw(*widget, states);
    if (!clockText.empty())
        target.draw(label(font, unicode(clockText), 14, contentWidth - 300, -20, Theme::muted),
                    states);
    if (!status.empty()) {
        rectangle(target, states, sf::Vector2f(statusWidth, statusLines * 18.f + 8), Theme::pale,
                  statusPosition - sf::Vector2f(0, 6));
        target.draw(label(font, wrap(font, unicode(status), 14, statusWidth - 24, statusLines), 14,
                          statusPosition.x + 12, statusPosition.y, Theme::ink),
                    states);
    }
    if (!dialog.empty()) {
        float left = (contentWidth - 560) / 2;
        rectangle(target, states, sf::Vector2f(contentWidth, 620), sf::Color(32, 56, 49, 140));
        rectangle(target, states, sf::Vector2f(560, 240), Theme::paper, sf::Vector2f(left, 160),
                  Theme::line);
        target.draw(label(font, wrap(font, unicode(dialog), 20, 500, 5), 20, left + 30, 190),
                    states);
        rectangle(target, states, sf::Vector2f(180, 44), Theme::accent,
                  sf::Vector2f(left + 60, 330));
        rectangle(target, states, sf::Vector2f(180, 44), Theme::pale,
                  sf::Vector2f(left + 260, 330));
        target.draw(label(font, unicode("Да · Enter"), 18, left + 80, 341, Theme::white), states);
        target.draw(label(font, unicode("Отмена · Esc"), 18, left + 280, 341), states);
    }
}
} // namespace ui
} // namespace exam
