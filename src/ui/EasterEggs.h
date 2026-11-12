#ifndef SMARTEXAM_EASTER_EGGS_H
#define SMARTEXAM_EASTER_EGGS_H
#include "Theme.h"
#include <cmath>
namespace exam {
namespace ui {
class EasterEggs {
    int clicks = 0;
    bool four = false;
    float lastClick = -10, lastKey = -10, planeAt = -10, answerAt = -10;

  public:
    void reset() {
        clicks = 0;
        four = false;
        planeAt = answerAt = -10;
    }
    bool planeVisible(float now) const {
        return now >= planeAt && now - planeAt < 2.4f;
    }
    bool answerVisible(float now) const {
        return now >= answerAt && now - answerAt < 4.f;
    }
    bool handle(const sf::Event &event, sf::Vector2f mouse, sf::FloatRect logo, bool allowed,
                bool editing, float now) {
        if (!allowed || event.type == sf::Event::LostFocus) {
            reset();
            return false;
        }
        if (editing)
            four = false;
        if (event.type == sf::Event::MouseButtonPressed) {
            four = false;
            if (event.mouseButton.button == sf::Mouse::Left && logo.contains(mouse)) {
                clicks = now - lastClick <= 2.f ? clicks + 1 : 1;
                lastClick = now;
                if (clicks == 7) {
                    planeAt = now;
                    clicks = 0;
                }
                return true;
            }
            clicks = 0;
        }
        if (!editing && event.type == sf::Event::TextEntered) {
            if (event.text.unicode == '2' && four && now - lastKey <= 1.5f) {
                answerAt = now;
                four = false;
            } else
                four = event.text.unicode == '4';
            lastKey = now;
        }
        if (event.type == sf::Event::KeyPressed &&
            (event.key.code == sf::Keyboard::Escape || event.key.code == sf::Keyboard::Tab ||
             event.key.control || event.key.alt || event.key.system)) {
            four = false;
            if (event.key.code == sf::Keyboard::Escape)
                reset();
        }
        return false;
    }
    void draw(sf::RenderTarget &target, const sf::Font &font, float now) const {
        const auto size = target.getView().getSize();
        if (planeVisible(now)) {
            const float p = (now - planeAt) / 2.4f;
            const float x = motionEnabled() ? 40 + (size.x + 60) * p : size.x / 2;
            const float y = motionEnabled() ? 82 - 26 * std::sin(p * 3.141593f) : 50;
            sf::ConvexShape wing(3);
            wing.setPoint(0, sf::Vector2f(-42, -19));
            wing.setPoint(1, sf::Vector2f(35, -5));
            wing.setPoint(2, sf::Vector2f(-18, 20));
            wing.setPosition(x, y);
            wing.setFillColor(Theme::white);
            wing.setOutlineColor(Theme::ink);
            wing.setOutlineThickness(1.5f);
            target.draw(wing);
            sf::ConvexShape fold(3);
            fold.setPoint(0, sf::Vector2f(-18, 20));
            fold.setPoint(1, sf::Vector2f(35, -5));
            fold.setPoint(2, sf::Vector2f(-8, 3));
            fold.setPosition(x, y);
            fold.setFillColor(Theme::pale);
            fold.setOutlineColor(Theme::ink);
            fold.setOutlineThickness(1.f);
            target.draw(fold);
            drawText(target, label(font, unicode("x²"), 12, x - 22, y - 15));
        }
        if (answerVisible(now)) {
            float width = std::min(620.f, size.x - 48);
            sf::Vector2f position((size.x - width) / 2, size.y - 66);
            rectangle(target, sf::RenderStates(), sf::Vector2f(width, 46), Theme::navigation,
                      position);
            drawText(target, label(font, unicode("Ответ найден. Осталось найти вопрос."), 19,
                                   position.x + 24, position.y + 10, Theme::white));
        }
    }
};
} // namespace ui
} // namespace exam
#endif
