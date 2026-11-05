#include "App.h"
#include "screens/HomeScreen.h"
#include "screens/WelcomeScreen.h"
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
    window.create(sf::VideoMode(1200, 800), "SmartExam");
    window.setFramerateLimit(60);
    screens["welcome"].reset(new WelcomeScreen(font, context));
    screens["home"].reset(new HomeScreen(font, context));
    screens["lesson"].reset(new LessonScreen(font, context));
    screens["bank"].reset(new BankScreen(font, context));
    screens["knowledge"].reset(new KnowledgeScreen(font, context));
    screens["packages"].reset(new PackageScreen(font, context));
    screens["results"].reset(new ResultsScreen(font, context));
    for (auto &item : screens) {
        item.second->setPosition(130, 130);
        item.second->setHeadingFont(headingFont);
    }
    screens["home"]->setPosition(52, 130);
    screens["lesson"]->setPosition(208, 130);
    show("welcome");
}
void App::buildNavigation() {
    navigation.clear();
    navigationPages.clear();
    if (context.role == UserRole::Guest)
        return;
    std::vector<std::string> names;
    if (context.role == UserRole::Student) {
        navigationPages = {"home", "knowledge", "results", "welcome"};
        names = {"Моё занятие", "Мои навыки", "История", "Сменить профиль"};
    } else {
        navigationPages = {"home", "bank", "knowledge", "packages", "welcome"};
        names = {"Кабинет", "Задания", "Навыки", "Пакеты", "Сменить роль"};
    }
    for (std::size_t i = 0; i < names.size(); ++i) {
        std::unique_ptr<Button> button(new Button(font, names[i], 167));
        button->setPosition(313 + i * 167.f, 16);
        button->setNavigation(true);
        std::string id = navigationPages[i];
        button->onClick = [this, id] { context.page = id; };
        bool selected = page == id || (page == "lesson" && id == "home") ||
                        (context.role == UserRole::Teacher && page == "results" && id == "home");
        button->setSelected(selected);
        navigation.push_back(std::move(button));
    }
}
void App::show(const std::string &requested) {
    window.setActive(true);
    std::string name = canOpen(context.role, requested) ? requested : "welcome";
    if (name == "welcome") {
        context.selectProfile("", "");
        context.role = UserRole::Guest;
    }
    page = name;
    current = screens.at(name).get();
    if (name == "welcome")
        static_cast<WelcomeScreen *>(current)->refresh();
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
    buildNavigation();
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
    window.draw(label(font, unicode("SmartExam"), 25, 52, 27));
    rectangle(window, sf::RenderStates(), sf::Vector2f(1096, 1), Theme::line,
              sf::Vector2f(52, 66));
    for (const auto &button : navigation)
        window.draw(*button);
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
        auto save = [this](const std::string &name) {
            draw();
            sf::Texture texture;
            texture.create(window.getSize().x, window.getSize().y);
            texture.update(window);
            texture.copyToImage().saveToFile("bin/captures/" + name + ".png");
        };
        show("welcome");
        save("welcome");
        context.role = UserRole::Student;
        context.selectProfile("demo-student", "Демонстрация");
        show("home");
        save("home");
        SessionSettings settings;
        settings.taskLimit = 3;
        context.school.start(context.profile, settings,
                             {"intro-fractions", "intro-linear", "intro-parabola"});
        context.settings = settings;
        context.current = context.school.next();
        show("lesson");
        save("lesson");
        show("knowledge");
        save("knowledge");
        show("results");
        save("results");
        context.role = UserRole::Teacher;
        show("home");
        save("teacher");
        show("bank");
        save("bank");
        show("packages");
        save("packages");
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
            if (!current->hasDialog())
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
