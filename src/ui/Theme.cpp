#include "Theme.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>
namespace exam {
namespace ui {
namespace {
float density = 1.f;
bool motion = false;
} // namespace
void setMotionEnabled(bool enabled) {
    motion = enabled;
}
bool motionEnabled() {
    return motion;
}
void setRenderScale(float value) {
    density = std::max(0.5f, std::min(4.f, value));
}
float renderScale() {
    return density;
}
void prepareCanvas(sf::RenderTexture &canvas, sf::Vector2f size) {
    sf::Vector2u pixels(static_cast<unsigned>(std::max(1.f, std::round(size.x * density))),
                        static_cast<unsigned>(std::max(1.f, std::round(size.y * density))));
    if (canvas.getSize() != pixels && !canvas.create(pixels.x, pixels.y))
        throw std::runtime_error("Не удалось создать поверхность отрисовки");
    canvas.setView(sf::View(sf::FloatRect(0, 0, pixels.x / density, pixels.y / density)));
    canvas.setSmooth(false);
}
sf::Sprite canvasSprite(const sf::RenderTexture &canvas) {
    sf::Sprite sprite(canvas.getTexture());
    sprite.setScale(1.f / density, 1.f / density);
    return sprite;
}
sf::Text sharpText(const sf::Text &source, float zoom) {
    sf::Text result = source;
    float logicalSize = std::round(source.getCharacterSize() * source.getScale().y);
    unsigned pixels = std::max(1u, static_cast<unsigned>(std::round(logicalSize * density * zoom)));
    auto position = source.getPosition();
    float top = source.getLocalBounds().top * source.getScale().y;
    result.setCharacterSize(pixels);
    result.setScale(1.f / (density * zoom), 1.f / (density * zoom));
    result.setPosition(position.x,
                       position.y + top - result.getLocalBounds().top * result.getScale().y);
    return result;
}
namespace {
sf::Vector2f alignedPosition(sf::RenderTarget &target, const sf::Transform &transform,
                             sf::Vector2f position) {
    sf::Vector2f world = transform.transformPoint(position);
    const sf::IntRect viewport = target.getViewport(target.getView());
    sf::Vector2f normalized = target.getView().getTransform().transformPoint(world);
    sf::Vector2i pixel(
        static_cast<int>(std::round(viewport.left + (normalized.x + 1) * viewport.width / 2)),
        static_cast<int>(std::round(viewport.top + (1 - normalized.y) * viewport.height / 2)));
    return transform.getInverse().transformPoint(target.mapPixelToCoords(pixel));
}
} // namespace
void drawText(sf::RenderTarget &target, const sf::Text &source, sf::RenderStates states) {
    const sf::Vector2f origin = states.transform.transformPoint(0, 0);
    const sf::Vector2f axis = states.transform.transformPoint(0, 1) - origin;
    const float scale = target.getViewport(target.getView()).height / target.getView().getSize().y *
                        std::sqrt(axis.x * axis.x + axis.y * axis.y);
    sf::Text text = sharpText(source, scale / density);
    text.setPosition(alignedPosition(target, states.transform, text.getPosition()));
    target.draw(text, states);
}
void drawCanvas(sf::RenderTarget &target, sf::Sprite sprite, sf::RenderStates states) {
    sprite.setPosition(alignedPosition(target, states.transform, sprite.getPosition()));
    target.draw(sprite, states);
}
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
    return sharpText(result);
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
