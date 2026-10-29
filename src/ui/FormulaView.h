#ifndef SMARTEXAM_FORMULA_VIEW_H
#define SMARTEXAM_FORMULA_VIEW_H
#include "widgets/Widget.h"
#include "math/parser/Parser.h"
namespace exam {
namespace ui {
class FormulaView : public Widget {
    struct Box {
        float width, height, baseline;
        std::vector<sf::Text> texts;
        std::vector<sf::RectangleShape> lines;
        Box() : width(0), height(0), baseline(0) {}
    };
    Box formula;
    std::string error;
    float zoom, scroll;
    bool featured;
    mutable sf::RenderTexture canvas;
    Box word(const std::string &text, unsigned size) const;
    void append(Box &target, const Box &child, float x, float y) const;
    Box horizontal(const Box &left, const Box &right) const;
    Box layout(const Expr &expression, unsigned size) const;
    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;

  public:
    FormulaView(const sf::Font &font, float width, float height = 130);
    void setFormula(const std::string &source);
    void setFeatured(bool value) {
        featured = value;
        zoom = 1.f;
    }
    std::string errorMessage() const {
        return error;
    }
    bool handle(const sf::Event &event, sf::Vector2f mouse) override;
};
} // namespace ui
} // namespace exam
#endif
