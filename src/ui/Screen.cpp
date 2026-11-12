#include "Screen.h"
namespace exam {
namespace ui {
void Screen::clear() {
    widgets.clear();
    captions.clear();
    captionLayouts.clear();
    widgetLayouts.clear();
    layoutWidth = 0;
    widthFactor = 1;
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
void Screen::caption(const sf::Font &face, const std::string &value, unsigned size, float x,
                     float y, float width, int lines, sf::Color color) {
    captionLayouts.push_back({&face, unicode(value), size, x, y, width, lines, color});
    captions.push_back(
        label(face, lines ? wrap(face, unicode(value), size, width, lines) : unicode(value), size,
              x, y, color));
    layoutWidth = 0;
}
void Screen::layout(float availableWidth) {
    if (layoutWidth == availableWidth)
        return;
    layoutWidth = availableWidth;
    widthFactor = availableWidth / contentWidth;
    for (std::size_t i = 0; i < widgets.size(); ++i) {
        widgets[i]->setPosition(widgetLayouts[i].position.x * widthFactor,
                                widgetLayouts[i].position.y);
        widgets[i]->setWidth(widgetLayouts[i].size.x * widthFactor);
    }
    for (std::size_t i = 0; i < captions.size(); ++i) {
        const auto &item = captionLayouts[i];
        sf::String value = item.lines ? wrap(*item.face, item.value, item.size,
                                             item.width * widthFactor, item.lines)
                                      : item.value;
        captions[i] = label(*item.face, value, item.size, item.x * widthFactor, item.y, item.color);
    }
}
void Screen::heading(const std::string &value, float x, float y, unsigned size) {
    caption(*headingFont, value, size, x, y, 0, 0, Theme::ink);
}
void Screen::note(const std::string &value, float x, float y, float width, sf::Color color) {
    caption(font, value, 14, x, y, width, 4, color);
}
void Screen::surface(float x, float y, float width, float height, sf::Color color) {
    sf::RectangleShape shape(sf::Vector2f(width, height));
    shape.setPosition(x, y);
    shape.setFillColor(color);
    surfaces.push_back(shape);
}
void Screen::text(const std::string &value, float x, float y, unsigned size, float width,
                  int lines) {
    caption(font, value, size, x, y, width, lines, Theme::ink);
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
            float left = (contentWidth * widthFactor - 560) / 2;
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
    drawText(target,
             label(*headingFont,
                   shorten(*headingFont, unicode(title), 32, contentWidth * widthFactor), 32, 0,
                   -57),
             states);
    for (auto surface : surfaces) {
        surface.setPosition(surface.getPosition().x * widthFactor, surface.getPosition().y);
        surface.setSize(sf::Vector2f(surface.getSize().x * widthFactor, surface.getSize().y));
        target.draw(surface, states);
    }
    for (const auto &caption : captions)
        drawText(target, caption, states);
    for (const auto &widget : widgets)
        if (widget->isVisible())
            target.draw(*widget, states);
    if (!clockText.empty())
        drawText(target,
                 label(font, unicode(clockText), 14, contentWidth * widthFactor - 300, -20,
                       Theme::muted),
                 states);
    if (!status.empty()) {
        rectangle(target, states, sf::Vector2f(statusWidth * widthFactor, statusLines * 18.f + 8),
                  Theme::pale, sf::Vector2f(statusPosition.x * widthFactor, statusPosition.y - 6));
        drawText(target,
                 label(font,
                       wrap(font, unicode(status), 14, statusWidth * widthFactor - 24, statusLines),
                       14, statusPosition.x * widthFactor + 12, statusPosition.y, Theme::ink),
                 states);
    }
    if (!dialog.empty()) {
        float left = (contentWidth * widthFactor - 560) / 2;
        rectangle(target, states, sf::Vector2f(contentWidth * widthFactor, 620),
                  sf::Color(32, 56, 49, 140));
        rectangle(target, states, sf::Vector2f(560, 240), Theme::paper, sf::Vector2f(left, 160),
                  Theme::line);
        drawText(target, label(font, wrap(font, unicode(dialog), 20, 500, 5), 20, left + 30, 190),
                 states);
        rectangle(target, states, sf::Vector2f(180, 44), Theme::accent,
                  sf::Vector2f(left + 60, 330));
        rectangle(target, states, sf::Vector2f(180, 44), Theme::pale,
                  sf::Vector2f(left + 260, 330));
        drawText(target, label(font, unicode("Да · Enter"), 18, left + 80, 341, Theme::white),
                 states);
        drawText(target, label(font, unicode("Отмена · Esc"), 18, left + 280, 341), states);
    }
}
} // namespace ui
} // namespace exam
