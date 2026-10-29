#include <SFML/Graphics.hpp>
#include <string>
#include "application/Demo.h"
#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char **argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    if (argc > 1 && std::string(argv[1]) == "--demo")
        return exam::runDemo();
    const bool smoke = argc > 1 && std::string(argv[1]) == "--smoke";
    sf::RenderWindow window(sf::VideoMode(960, 640), "SmartExam");
    window.setFramerateLimit(60);
    sf::Clock clock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }
        window.clear(sf::Color(240, 243, 248));
        window.display();
        if (smoke && clock.getElapsedTime().asSeconds() > 1)
            window.close();
    }
    return 0;
}
