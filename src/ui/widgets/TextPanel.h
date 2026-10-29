#ifndef SMARTEXAM_TEXT_PANEL_H
#define SMARTEXAM_TEXT_PANEL_H
#include "Widget.h"
namespace exam {
namespace ui {
class TextPanel : public Widget {
    sf::Text text;
    float scroll;
    mutable sf::RenderTexture canvas;
    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;

  public:
    TextPanel(const sf::Font &font, const std::string &value, float width, float height);
    bool handle(const sf::Event &event, sf::Vector2f mouse) override;
};
} // namespace ui
} // namespace exam
#endif
