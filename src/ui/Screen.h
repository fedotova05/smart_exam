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
    struct CaptionLayout {
        const sf::Font *face;
        sf::String value;
        unsigned size;
        float x, y, width;
        int lines;
        sf::Color color;
    };
    struct WidgetLayout {
        sf::Vector2f position, size;
    };
    std::vector<CaptionLayout> captionLayouts;
    std::vector<WidgetLayout> widgetLayouts;
    float layoutWidth = 0, widthFactor = 1;
    void caption(const sf::Font &face, const std::string &value, unsigned size, float x, float y,
                 float width, int lines, sf::Color color);
    std::vector<sf::RectangleShape> surfaces;
    const sf::Font *headingFont;
    int focus;
    std::function<void()> pending, confirmation;
    std::string dialog;
    sf::Vector2f statusPosition;
    float contentWidth, statusWidth;
    float titleWidth = 0;
    int statusLines;
    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;

  public:
    const sf::Font &font;
    std::string title, status, clockText;
    explicit Screen(const sf::Font &face)
        : headingFont(&face), focus(-1), statusPosition(0, 628), contentWidth(940),
          statusWidth(930), statusLines(2), font(face) {}
    virtual ~Screen() {}
    virtual void tick() {}
    virtual void leave() {}
    void clear();
    void layout(float availableWidth);
    void focusWidget(Widget *widget);
    bool moveFocus(bool backward, bool wrap = true);
    void clearFocus();
    void setContentWidth(float width) {
        contentWidth = width;
        layoutWidth = 0;
    }
    void setTitleWidth(float width) {
        titleWidth = width;
    }
    void setStatusArea(float x, float y, float width, int lines = 2) {
        statusPosition = sf::Vector2f(x, y);
        statusWidth = width;
        statusLines = lines;
    }
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
    void progressBar(float value, float x, float y, float width);
    void text(const std::string &value, float x, float y, unsigned size = 18, float width = 900,
              int lines = 3);
    template <class T> T *add(T *widget, float x, float y) {
        widget->setPosition(x, y);
        widgets.emplace_back(widget);
        widgetLayouts.push_back({sf::Vector2f(x, y), widget->getSize()});
        layoutWidth = 0;
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
    bool textInputFocused() const;
    bool hasDialog() const {
        return !dialog.empty();
    }
    virtual void handle(const sf::Event &event, sf::Vector2f mouse);
    void update();
};
} // namespace ui
} // namespace exam
#endif
