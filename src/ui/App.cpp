#include "App.h"
#include "screens/HomeScreen.h"
#include "screens/LessonScreen.h"
#include "screens/BankScreen.h"
#include "screens/KnowledgeScreen.h"
#include "screens/PackageScreen.h"
#include "FormulaView.h"
#include "io/Files.h"
#include <windows.h>
namespace exam {
namespace ui {
App::App(SchoolService &school) : context(school), current(0) {
    char directory[MAX_PATH];
    GetWindowsDirectoryA(directory, MAX_PATH);
    std::string fonts = std::string(directory) + "/Fonts/";
    if (!font.loadFromFile(fonts + "segoeui.ttf") && !font.loadFromFile(fonts + "arial.ttf"))
        throw std::runtime_error("Не найден шрифт Segoe UI или Arial в папке Windows/Fonts");
    if (!headingFont.loadFromFile(fonts + "georgia.ttf"))
        headingFont = font;
    for (const auto &profile : school.profiles())
        if (profile.first == "demo-student") {
            context.profile = profile.first;
            context.profileName = profile.second;
        }
    window.create(sf::VideoMode(1200, 800), "SmartExam");
    window.setFramerateLimit(60);
    screens["home"].reset(new HomeScreen(font, context));
    screens["lesson"].reset(new LessonScreen(font, context));
    screens["bank"].reset(new BankScreen(font, context));
    screens["knowledge"].reset(new KnowledgeScreen(font, context));
    screens["packages"].reset(new PackageScreen(font, context));
    screens["results"].reset(new ResultsScreen(font, context));
    const char *ids[] = {"home", "lesson", "bank", "knowledge", "results", "packages"};
    const char *titles[] = {"Главная",      "Решение",    "Банк заданий",
                            "Карта знаний", "Результаты", "Пакеты"};
    for (int i = 0; i < 6; ++i) {
        std::unique_ptr<Button> button(new Button(font, titles[i], 178));
        button->setPosition(16, 160 + i * 52.f);
        button->setNavigation(true);
        std::string id = ids[i];
        button->onClick = [this, id] { context.page = id; };
        navigation.push_back(std::move(button));
    }
    for (auto &item : screens) {
        item.second->setPosition(236, 130);
        item.second->setHeadingFont(headingFont);
    }
    show("home");
}
void App::show(const std::string &name) {
    window.setActive(true);
    page = name;
    current = screens.at(name).get();
    if (name == "home")
        static_cast<HomeScreen *>(current)->refresh();
    if (name == "lesson")
        static_cast<LessonScreen *>(current)->refresh();
    if (name == "bank")
        static_cast<BankScreen *>(current)->refresh();
    if (name == "knowledge")
        static_cast<KnowledgeScreen *>(current)->refresh();
    if (name == "packages")
        static_cast<PackageScreen *>(current)->refresh();
    if (name == "results")
        static_cast<ResultsScreen *>(current)->refresh();
    const char *ids[] = {"home", "lesson", "bank", "knowledge", "results", "packages"};
    for (std::size_t i = 0; i < navigation.size(); ++i) {
        navigation[i]->setFocus(false);
        navigation[i]->setSelected(name == ids[i]);
    }
}
void App::resize(unsigned width, unsigned height) {
    if (width < 1024 || height < 720) {
        window.setSize(sf::Vector2u(std::max(1024u, width), std::max(720u, height)));
        return;
    }
    sf::View view(sf::FloatRect(0, 0, 1200, 800));
    float ratio = static_cast<float>(width) / height, target = 1.5f;
    if (ratio > target)
        view.setViewport(sf::FloatRect((1 - target / ratio) / 2, 0, target / ratio, 1));
    else
        view.setViewport(sf::FloatRect(0, (1 - ratio / target) / 2, 1, ratio / target));
    window.setView(view);
}
void App::draw() {
    window.clear(Theme::background);
    rectangle(window, sf::RenderStates(), sf::Vector2f(210, 800), Theme::navigation);
    window.draw(label(font, unicode("SmartExam"), 25, 25, 35, Theme::white));
    rectangle(window, sf::RenderStates(), sf::Vector2f(4, 4), Theme::accent, sf::Vector2f(170, 57));
    window.draw(
        label(font, unicode("УЧЕБНОЕ ПРОСТРАНСТВО"), 10, 25, 125, sf::Color(147, 170, 153)));
    window.draw(label(font, unicode("Математика  /  Практика"), 13, 236, 29, Theme::muted));
    window.draw(label(font, unicode(context.profileName), 13, 980, 29, Theme::muted));
    rectangle(window, sf::RenderStates(), sf::Vector2f(940, 1), Theme::line, sf::Vector2f(236, 61));
    for (const auto &button : navigation)
        window.draw(*button);
    window.draw(label(
        font,
        wrap(font, unicode(context.profileName.empty() ? "Выберите профиль" : context.profileName),
             16, 170, 3),
        16, 25, 694, Theme::white));
    rectangle(window, sf::RenderStates(), sf::Vector2f(160, 1), sf::Color(72, 96, 81),
              sf::Vector2f(25, 670));
    window.draw(label(
        font, unicode(context.profile == "demo-student" ? "Учебные данные" : "Локальный профиль"),
        12, 25, 730, sf::Color(147, 170, 153)));
    window.draw(*current);
    window.display();
}
int App::run(bool smoke, bool capture, bool widgets) {
    if (widgets) {
        std::unique_ptr<Screen> demo(new Screen(font));
        demo->title = "Элементы интерфейса";
        demo->setPosition(236, 100);
        TextField *field = demo->field("Кириллица: Анастасия", 0, 0, 500);
        demo->list(
            {"Первая строка", "Вторая строка", "Третья строка", "Четвёртая строка", "Пятая строка"},
            0, 70, 500, 120);
        demo->add(new Toggle(font, "Переключатель", 240), 0, 220);
        FormulaView *formula = demo->add(new FormulaView(font, 600, 170), 0, 300);
        formula->setFormula("(x+1)/(x^2+sqrt(3))");
        field->onChange = [field, formula] { formula->setFormula(field->getText()); };
        Screen *screen = demo.get();
        demo->button("Диалог", 280, 220, 220, [screen] {
            screen->ask("Пример подтверждения", [screen] { screen->status = "Подтверждено"; });
        });
        screens["widgets"] = std::move(demo);
        current = screens["widgets"].get();
    }
    if (capture) {
        makeDirectories("bin/captures");
        SessionSettings settings;
        settings.taskLimit = 1;
        const char *pages[] = {"home", "lesson", "bank", "knowledge", "packages", "results"};
        for (const char *item : pages) {
            if (std::string(item) == "lesson") {
                context.school.start(context.profile, settings, {"intro-quadratic"});
                context.current = context.school.next();
            }
            show(item);
            draw();
            sf::Texture texture;
            texture.create(1200, 800);
            texture.update(window);
            texture.copyToImage().saveToFile(std::string("bin/captures/") + item + ".png");
        }
        return 0;
    }
    sf::Clock clock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                break;
            }
            if (event.type == sf::Event::Resized)
                resize(event.size.width, event.size.height);
            sf::Vector2i pixel = sf::Mouse::getPosition(window);
            if (event.type == sf::Event::MouseButtonPressed ||
                event.type == sf::Event::MouseButtonReleased)
                pixel = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
            if (event.type == sf::Event::MouseMoved)
                pixel = sf::Vector2i(event.mouseMove.x, event.mouseMove.y);
            sf::Vector2f mouse = window.mapPixelToCoords(pixel);
            for (auto &button : navigation)
                button->handle(event, mouse);
            current->handle(event, mouse);
            current->update();
            if (!context.page.empty()) {
                std::string next = context.page;
                context.page.clear();
                try {
                    show(next);
                } catch (const std::exception &error) {
                    current->status = error.what();
                }
            }
        }
        if (!window.isOpen())
            break;
        draw();
        if (smoke && clock.getElapsedTime().asSeconds() > 1)
            window.close();
    }
    return 0;
}
} // namespace ui
} // namespace exam
