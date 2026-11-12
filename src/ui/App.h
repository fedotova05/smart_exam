#ifndef SMARTEXAM_APP_H
#define SMARTEXAM_APP_H
#include "AppContext.h"
#include "Screen.h"
#include "EasterEggs.h"
#include <memory>
namespace exam {
namespace ui {
class App {
    sf::Context graphicsContext;
    sf::RenderWindow window;
    EasterEggs easterEggs;
    sf::Clock decorationClock;
    sf::Font font;
    sf::Font headingFont;
    AppContext context;
    std::map<std::string, std::unique_ptr<Screen>> screens;
    std::vector<std::unique_ptr<Button>> navigation;
    std::vector<std::string> navigationPages;
    Screen *current;
    std::string page;
    int navigationFocus;
    bool fullscreen = false;
    std::unique_ptr<Button> windowMode, closeButton;
    void toggleFullscreen();
    void show(const std::string &name);
    void buildNavigation();
    void draw();
    void layoutScreen();
    void resize(unsigned width, unsigned height);

  public:
    explicit App(SchoolService &school);
    int run(bool smoke = false, bool capture = false, bool widgets = false);
};
} // namespace ui
} // namespace exam
#endif
