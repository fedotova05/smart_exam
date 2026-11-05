#include "App.h"
#include "screens/HomeScreen.h"
#include "screens/TrainingSetupScreen.h"
#include "screens/TrainingDemoScreen.h"
#include "screens/LessonSetupScreen.h"
#include "screens/ProgressScreen.h"
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
    screens["practice"].reset(new TrainingSetupScreen(font, context));
    screens["training-demo"].reset(new TrainingDemoScreen(font, context));
    screens["setup"].reset(new LessonSetupScreen(font, context));
    screens["progress"].reset(new ProgressScreen(font, context));
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
        navigationPages = {"home", "progress", "results", "welcome"};
        names = {"Моё занятие", "Мой прогресс", "История", "Сменить профиль"};
    } else {
        navigationPages = {"home", "bank", "progress", "packages", "welcome"};
        names = {"Кабинет", "Задания", "Прогресс", "Пакеты", "Сменить роль"};
    }
    for (std::size_t i = 0; i < names.size(); ++i) {
        std::unique_ptr<Button> button(new Button(font, names[i], 167));
        button->setPosition(313 + i * 167.f, 16);
        button->setNavigation(true);
        std::string id = navigationPages[i];
        button->onClick = [this, id] { context.page = id; };
        bool selected =
            page == id ||
            ((page == "lesson" || page == "setup" || page == "practice") && id == "home") ||
            (page == "knowledge" && id == "progress") ||
            (context.role == UserRole::Teacher && page == "results" && id == "home");
        button->setSelected(selected);
        navigation.push_back(std::move(button));
    }
}
void App::show(const std::string &requested) {
    if (current)
        current->leave();
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
    if (name == "practice")
        static_cast<TrainingSetupScreen *>(current)->refresh();
    if (name == "training-demo")
        static_cast<TrainingDemoScreen *>(current)->refresh();
    if (name == "setup")
        static_cast<LessonSetupScreen *>(current)->refresh();
    if (name == "progress")
        static_cast<ProgressScreen *>(current)->refresh();
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
    // Не растягиваем готовые текстуры при увеличении окна.
    float scale = std::min(1.f, std::min(width / 1200.f, height / 800.f));
    float viewportWidth = 1200.f * scale / width;
    float viewportHeight = 800.f * scale / height;
    view.setViewport(sf::FloatRect((1 - viewportWidth) / 2, (1 - viewportHeight) / 2, viewportWidth,
                                   viewportHeight));
    window.setView(view);
}
void App::draw() {
    window.clear(Theme::background);
    window.draw(label(font, unicode("SmartExam"), 25, 52, 27));
    rectangle(window, sf::RenderStates(), sf::Vector2f(1096, 1), Theme::line, sf::Vector2f(52, 66));
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
        auto click = [this](float x, float y) {
            sf::Event event = {};
            event.type = sf::Event::MouseButtonPressed;
            event.mouseButton.button = sf::Mouse::Left;
            sf::Vector2f point = current->getPosition() + sf::Vector2f(x, y);
            current->handle(event, point);
            current->update();
            event.type = sf::Event::MouseButtonReleased;
            current->handle(event, point);
            current->update();
        };
        show("welcome");
        save("welcome");
        context.role = UserRole::Student;
        context.selectProfile("demo-student", "Демонстрация");
        show("home");
        save("home");
        show("practice");
        save("training-setup");
        click(50, 210);
        save("training-topics-selected");
        show("training-demo");
        save("training-demo");
        context.selectProfile("demo-training", "Демонстрация: разбор ошибки");
        show("knowledge");
        save("training-skill-map");
        context.selectProfile("demo-student", "Демонстрация");
        show("setup");
        save("setup-bank");
        click(230, 90);
        save("setup-generator");
        click(60, 490);
        save("setup-basket");
        click(680, 595);
        save("setup-review");
        show("progress");
        save("progress-demo");
        context.selectProfile(context.school.createProfile("Мария"), "Мария");
        show("home");
        save("empty-profile");
        SessionSettings settings;
        settings.taskLimit = 3;
        context.school.start(context.profile, settings,
                             {"intro-fractions", "intro-linear", "intro-parabola"});
        context.settings = settings;
        context.current = context.school.next();
        show("lesson");
        save("lesson");
        AnswerPayload answer;
        answer.text = "5/6";
        context.school.submit(context.current.attempt.id, "a", answer, true);
        context.school.finish(context.current.attempt.id);
        context.current.attempt.finished = true;
        show("lesson");
        save("task-result");
        show("home");
        save("progress-example");
        context.current = context.school.next();
        show("lesson");
        save("lesson-route");
        click(660, 185);
        save("linear-optional-steps");
        show("knowledge");
        save("knowledge");
        show("progress");
        save("progress-one-answer");
        show("results");
        save("results");
        context.role = UserRole::Teacher;
        show("home");
        save("teacher");
        show("bank");
        save("bank");
        click(300, 460);
        click(80, 580);
        save("answer-settings");
        show("packages");
        save("packages");
        context.role = UserRole::Student;
        context.selectProfile(context.school.createProfile("Отложенная проверка"),
                              "Отложенная проверка");
        settings.feedback = FeedbackPolicy::AfterSet;
        settings.taskLimit = 2;
        context.school.start(context.profile, settings, {"intro-fractions", "intro-linear"});
        context.settings = settings;
        context.current = context.school.next();
        context.school.submit(context.current.attempt.id, "a", answer, true);
        context.school.finish(context.current.attempt.id);
        show("home");
        save("hidden-progress");
        show("knowledge");
        save("hidden-skills");
        show("progress");
        save("hidden-progress-table");
        // Проверяем минимальное и широкое окно на тех же данных.
        window.setSize(sf::Vector2u(1024, 720));
        resize(1024, 720);
        show("welcome");
        save("welcome-1024");
        context.role = UserRole::Student;
        context.selectProfile("demo-student", "Демонстрация");
        show("setup");
        save("setup-1024");
        show("progress");
        save("progress-1024");
        window.setSize(sf::Vector2u(1600, 900));
        resize(1600, 900);
        context.role = UserRole::Student;
        context.selectProfile("demo-student", "Демонстрация");
        show("home");
        save("home-1600");
        show("setup");
        save("setup-1600");
        window.setSize(sf::Vector2u(1920, 1080));
        resize(1920, 1080);
        show("progress");
        save("progress-1920");
        window.setSize(sf::Vector2u(1200, 800));
        resize(1200, 800);
        context.selectProfile(context.school.createProfile("Квадратные уравнения"),
                              "Квадратные уравнения");
        settings.feedback = FeedbackPolicy::AfterStep;
        settings.guided = false;
        context.school.start(context.profile, settings, {"intro-quadratic"});
        context.settings = settings;
        context.current = context.school.next();
        show("lesson");
        save("quadratic-final-answer");
        context.selectProfile(context.school.createProfile("Тренировка"), "Тренировка");
        context.school.startTopics(context.profile, {"q_roots"}, true, 1);
        context.settings = context.school.latestLesson(context.profile).settings;
        context.current = context.school.next();
        show("lesson");
        context.drafts["answer"].text = "0";
        click(470, 340);
        save("training-support-choice");
        click(100, 475);
        save("training-coefficients");
        window.setSize(sf::Vector2u(1024, 720));
        resize(1024, 720);
        show("practice");
        save("training-setup-1024");
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
        try {
            current->tick();
        } catch (const std::exception &error) {
            current->status = error.what();
        }
        draw();
        if (smoke && clock.getElapsedTime().asSeconds() > 1)
            window.close();
    }
    return 0;
}
} // namespace ui
} // namespace exam
