#include "TextPanel.h"
#include <algorithm>
namespace exam {
namespace ui {
TextPanel::TextPanel(const sf::Font &font, const std::string &value, float width, float height)
    : Widget(font, width, height),
      text(label(font, wrap(font, unicode(value), 18, width - 24, 2000), 18, 10, 8)), scroll(0) {
    canvas.create(static_cast<unsigned>(width), static_cast<unsigned>(height));
}
bool TextPanel::handle(const sf::Event &event, sf::Vector2f mouse) {
    float delta = 0;
    if (event.type == sf::Event::MouseWheelScrolled && contains(mouse))
        delta = -event.mouseWheelScroll.delta * 35;
    if (event.type == sf::Event::KeyPressed && focused) {
        if (event.key.code == sf::Keyboard::Down)
            delta = 35;
        if (event.key.code == sf::Keyboard::Up)
            delta = -35;
    }
    scroll = std::max(
        0.f, std::min(std::max(0.f, text.getLocalBounds().height + 20 - size.y), scroll + delta));
    return delta != 0;
}
void TextPanel::draw(sf::RenderTarget &target, sf::RenderStates states) const {
    canvas.clear(Theme::background);
    sf::Text visibleText = text;
    visibleText.move(0, -scroll);
    canvas.draw(visibleText);
    if (text.getLocalBounds().height + 20 > size.y) {
        float total = text.getLocalBounds().height + 20;
        rectangle(canvas, sf::RenderStates(), sf::Vector2f(3, size.y * size.y / total),
                  Theme::accent, sf::Vector2f(size.x - 3, size.y * scroll / total));
    }
    canvas.display();
    states.transform *= getTransform();
    target.draw(sf::Sprite(canvas.getTexture()), states);
}
} // namespace ui
} // namespace exam
