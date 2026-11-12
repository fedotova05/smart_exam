#ifndef SMARTEXAM_TEXT_AREA_H
#define SMARTEXAM_TEXT_AREA_H
#include "Widget.h"
namespace exam {
namespace ui {
class TextArea : public Widget {
    sf::String value;
    std::size_t cursor = 0;
    bool all = false;
    mutable float scroll = 0;
    mutable sf::RenderTexture canvas;
    struct Line {
        std::size_t start, end;
    };
    std::vector<Line> lines() const;
    void insert(const sf::String &text);
    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;

  public:
    std::function<void()> onChange;
    TextArea(const sf::Font &font, float width, float height) : Widget(font, width, height) {}
    void setText(const std::string &text) {
        value = unicode(text);
        cursor = value.getSize();
        scroll = 0;
    }
    std::string getText() const {
        return utf8(value);
    }
    bool handle(const sf::Event &event, sf::Vector2f mouse) override;
};
} // namespace ui
} // namespace exam
#endif
