#include "ImageView.h"
#include "io/Files.h"
#include <algorithm>
namespace exam {
namespace ui {
ImageView::ImageView(const sf::Font &font, const std::string &path) : Widget(font, 940, 450) {
    std::string bytes = readFile(resourcePath("data", path), 5 * 1024 * 1024);
    if (!texture.loadFromMemory(bytes.data(), bytes.size()))
        throw std::runtime_error("Не удалось прочитать изображение");
    texture.setSmooth(true);
}
void ImageView::draw(sf::RenderTarget &target, sf::RenderStates states) const {
    states.transform *= getTransform();
    sf::Sprite sprite(texture);
    float scale = std::min(size.x / texture.getSize().x, size.y / texture.getSize().y);
    sprite.setScale(scale, scale);
    sprite.setPosition((size.x - texture.getSize().x * scale) / 2,
                       (size.y - texture.getSize().y * scale) / 2);
    target.draw(sprite, states);
}
} // namespace ui
} // namespace exam
