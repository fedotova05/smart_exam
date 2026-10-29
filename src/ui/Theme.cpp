#include "Theme.h"
namespace exam {
namespace ui {
sf::String unicode(const std::string &text) {
    return sf::String::fromUtf8(text.begin(), text.end());
}
std::string utf8(const sf::String &text) {
    auto bytes = text.toUtf8();
    return std::string(bytes.begin(), bytes.end());
}
sf::Text label(const sf::Font &font, const sf::String &text, unsigned size, float x, float y,
               sf::Color color) {
    sf::Text result(text, font, size);
    result.setFillColor(color);
    result.setPosition(x, y - result.getLocalBounds().top);
    return result;
}
sf::String shorten(const sf::Font &font, const sf::String &text, unsigned size, float width) {
    if (sf::Text(text, font, size).getLocalBounds().width <= width)
        return text;
    sf::String result = text;
    while (result.getSize() > 0 &&
           sf::Text(result + unicode("…"), font, size).getLocalBounds().width > width)
        result.erase(result.getSize() - 1);
    return result + unicode("…");
}
sf::String wrap(const sf::Font &font, const sf::String &text, unsigned size, float width,
                int maxLines) {
    sf::String result, current;
    int lines = 1;
    std::size_t position = 0;
    while (position < text.getSize()) {
        if (text[position] == '\n') {
            if (lines >= maxLines)
                return result + shorten(font, current + unicode("…"), size, width);
            result += current + "\n";
            current.clear();
            ++lines;
            ++position;
            continue;
        }
        if (text[position] == ' ' || text[position] == '\t') {
            ++position;
            continue;
        }
        sf::String word;
        while (position < text.getSize() && text[position] != ' ' && text[position] != '\n' &&
               text[position] != '\t')
            word += text[position++];
        sf::String next = current + (current.isEmpty() ? "" : " ") + word;
        if (sf::Text(next, font, size).getLocalBounds().width > width && !current.isEmpty()) {
            if (lines >= maxLines)
                return result + shorten(font, next, size, width);
            result += current + "\n";
            current.clear();
            ++lines;
        }
        if (!current.isEmpty())
            current += " ";
        current += shorten(font, word, size, width);
    }
    return result + current;
}
void rectangle(sf::RenderTarget &target, sf::RenderStates states, sf::Vector2f size, sf::Color fill,
               sf::Vector2f position, sf::Color border) {
    sf::RectangleShape shape(size);
    shape.setPosition(position);
    shape.setFillColor(fill);
    if (border.a) {
        shape.setOutlineColor(border);
        shape.setOutlineThickness(-1);
    }
    target.draw(shape, states);
}
} // namespace ui
} // namespace exam
