#ifndef SMARTEXAM_IMAGE_VIEW_H
#define SMARTEXAM_IMAGE_VIEW_H
#include "widgets/Widget.h"
namespace exam {
namespace ui {
class ImageView : public Widget {
    sf::Texture texture;
    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;

  public:
    ImageView(const sf::Font &font, const std::string &path);
    bool handle(const sf::Event &, sf::Vector2f) override {
        return false;
    }
    bool focusable() const override {
        return false;
    }
};
} // namespace ui
} // namespace exam
#endif
