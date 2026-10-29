#ifndef SMARTEXAM_THEME_H
#define SMARTEXAM_THEME_H
#include <SFML/Graphics.hpp>
#include <string>
namespace exam {
namespace ui {
namespace Theme {
const sf::Color background(247, 246, 240), paper(250, 249, 245), ink(30, 55, 47);
const sf::Color accent(208, 105, 77), muted(116, 131, 117), line(220, 224, 214);
const sf::Color pale(237, 240, 230), white(255, 254, 249), error(166, 63, 43);
const sf::Color navigation(30, 53, 46), navigationActive(54, 80, 68);
} // namespace Theme
sf::String unicode(const std::string &text);
std::string utf8(const sf::String &text);
sf::Text label(const sf::Font &font, const sf::String &text, unsigned size, float x, float y,
               sf::Color color = Theme::ink);
sf::String shorten(const sf::Font &font, const sf::String &text, unsigned size, float width);
sf::String wrap(const sf::Font &font, const sf::String &text, unsigned size, float width,
                int maxLines = 8);
void rectangle(sf::RenderTarget &target, sf::RenderStates states, sf::Vector2f size, sf::Color fill,
               sf::Vector2f position = sf::Vector2f(), sf::Color border = sf::Color::Transparent);
} // namespace ui
} // namespace exam
#endif
