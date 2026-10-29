#ifndef SMARTEXAM_SCREEN_H
#define SMARTEXAM_SCREEN_H
#include "widgets/Button.h"
#include "widgets/TextField.h"
#include "widgets/ListBox.h"
#include <memory>
namespace exam {
namespace ui {
class Screen : public sf::Drawable, public sf::Transformable {
    std::vector<std::unique_ptr<Widget>> widgets;
    std::vector<sf::Text> captions;
    std::vector<sf::RectangleShape> surfaces;
    const sf::Font *headingFont;
    int focus;
    std::function<void()> pending, confirmation;
    std::string dialog;
    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;

  public:
    const sf::Font &font;
    std::string title, status;
    explicit Screen(const sf::Font &face) : headingFont(&face), focus(-1), font(face) {}
    virtual ~Screen() {}
    void clear();
    void setHeadingFont(const sf::Font &face) {
        headingFont = &face;
    }
    const sf::Font &headingFace() const {
        return *headingFont;
    }
    void heading(const std::string &value, float x, float y, unsigned size = 36);
    void note(const std::string &value, float x, float y, float width = 900,
              sf::Color color = Theme::muted);
    void surface(float x, float y, float width, float height, sf::Color color);
    void text(const std::string &value, float x, float y, unsigned size = 18, float width = 900,
              int lines = 3);
    template <class T> T *add(T *widget, float x, float y) {
        widget->setPosition(x, y);
        widgets.emplace_back(widget);
        return widget;
    }
    Button *button(const std::string &title, float x, float y, float width,
                   const std::function<void()> &action, bool primary = false);
    TextField *field(const std::string &value, float x, float y, float width,
                     std::size_t limit = 512);
    ListBox *list(const std::vector<std::string> &rows, float x, float y, float width,
                  float height);
    void defer(const std::function<void()> &action) {
        pending = action;
    }
    void ask(const std::string &message, const std::function<void()> &action);
    void handle(const sf::Event &event, sf::Vector2f mouse);
    void update();
};
} // namespace ui
} // namespace exam
#endif
