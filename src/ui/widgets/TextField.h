#ifndef SMARTEXAM_TEXT_FIELD_H
#define SMARTEXAM_TEXT_FIELD_H
#include "Widget.h"
namespace exam {
namespace ui {
class TextField : public Widget {
    sf::String value, placeholder;
    std::size_t cursor, maximum;
    bool selectedAll;
    mutable sf::RenderTexture canvas;
    mutable float scroll;
    void changed();
    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;

  public:
    std::function<void()> onChange, onEnter;
    TextField(const sf::Font &font, float width = 250, std::size_t limit = 512);
    void setText(const std::string &text);
    std::string getText() const {
        return utf8(value);
    }
    void setPlaceholder(const std::string &text) {
        placeholder = unicode(text);
    }
    void insert(const std::string &text);
    std::size_t cursorPosition() const {
        return cursor;
    }
    bool handle(const sf::Event &event, sf::Vector2f mouse) override;
};
} // namespace ui
} // namespace exam
#endif
