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
App::App(SchoolService &school) : context(school), current(0), navigationFocus(-1) {
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
    screens["review"].reset(new HomeScreen(font, context));
    screens["profiles"].reset(new HomeScreen(font, context));
    screens["lesson"].reset(new LessonScreen(font, context));
    screens["bank"].reset(new BankScreen(font, context));
    screens["knowledge"].reset(new KnowledgeScreen(font, context));
    screens["packages"].reset(new PackageScreen(font, context));
    screens["results"].reset(new ResultsScreen(font, context));
    for (auto &item : screens) {
        item.second->setPosition(224, 130);
        item.second->setHeadingFont(headingFont);
    }
    screens["welcome"]->setPosition(130, 130);
    screens["lesson"]->setPosition(208, 130);
    show("welcome");
}
void App::buildNavigation() {
    navigation.clear();
    navigationPages.clear();
    navigationFocus = -1;
    if (context.role == UserRole::Guest || page == "lesson")
        return;
    std::vector<std::string> names;
    if (context.role == UserRole::Student) {
        navigationPages = {"home", "practice", "progress", "results", "knowledge", "welcome"};
        names = {"Моё занятие",     "Темы и режим",  "Мой прогресс",
                 "История решений", "Карта навыков", "Сменить профиль"};
    } else {
        navigationPages = {"home",     "setup",     "bank",     "review", "profiles",
                           "progress", "knowledge", "packages", "welcome"};
        names = {"Кабинет",  "Создать занятие", "Банк заданий",     "Проверить ответы", "Ученики",
                 "Прогресс", "Карта навыков",   "Импорт и экспорт", "Сменить роль"};
    }
    for (std::size_t i = 0; i < names.size(); ++i) {
        std::unique_ptr<Button> button(new Button(font, names[i], 180));
        button->setPosition(18, 126 + i * 54.f);
        button->setNavigation(true);
        std::string id = navigationPages[i];
        button->onClick = [this, id] { context.page = id; };
        button->setSelected(page == id);
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
        context.school.logoutTeacher();
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
    if (name == "review")
        static_cast<HomeScreen *>(current)->openReview();
    if (name == "profiles")
        static_cast<HomeScreen *>(current)->openProfiles();
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
    if (!navigation.empty()) {
        rectangle(window, sf::RenderStates(), sf::Vector2f(208, 800), Theme::paper);
        rectangle(window, sf::RenderStates(), sf::Vector2f(1, 800), Theme::line,
                  sf::Vector2f(207, 0));
        window.draw(label(font, unicode(context.role == UserRole::Teacher ? "УЧИТЕЛЬ" : "УЧЕНИК"),
                          13, 30, 94, Theme::muted));
        window.draw(label(font, unicode("Tab — переход по меню"), 12, 22, 749, Theme::muted));
    }
    window.draw(label(font, unicode("SmartExam"), 25, navigation.empty() ? 52 : 24, 27));
    if (page == "lesson") {
        LessonScreen *lesson = static_cast<LessonScreen *>(current);
        current->setPosition(lesson->focusLayout() ? 240.f : 130.f, 130);
    }
    if (context.role != UserRole::Guest) {
        std::string identity =
            context.role == UserRole::Teacher ? "Кабинет учителя" : context.profileName;
        if (context.role == UserRole::Teacher && !context.profileName.empty())
            identity += " / " + context.profileName;
        window.draw(
            label(font, shorten(font, unicode(identity), 15, 450), 15, 240, 30, Theme::muted));
    }
    rectangle(window, sf::RenderStates(), sf::Vector2f(940, 1), Theme::line, sf::Vector2f(224, 64));
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
        auto key = [this](sf::Keyboard::Key code) {
            sf::Event event = {};
            event.type = sf::Event::KeyPressed;
            event.key.code = code;
            current->handle(event, sf::Vector2f());
            current->update();
        };
        auto type = [this](const std::string &text) {
            for (sf::Uint32 c : unicode(text)) {
                sf::Event event = {};
                event.type = sf::Event::TextEntered;
                event.text.unicode = c;
                current->handle(event, sf::Vector2f());
                current->update();
            }
        };
        show("welcome");
        save("welcome");
        context.role = UserRole::Student;
        context.selectProfile("demo-check", "Демонстрация");
        show("home");
        save("home");
        show("practice");
        save("training-setup");
        click(15, 230);
        save("training-topics-selected");
        show("training-demo");
        save("training-demo");
        context.selectProfile("demo-training", "Демонстрация: разбор ошибки");
        show("knowledge");
        save("training-skill-map");
        context.selectProfile("demo-check", "Демонстрация");
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
        click(450, 525);
        save("linear-optional-steps");
        show("knowledge");
        save("knowledge");
        show("progress");
        save("progress-one-answer");
        show("results");
        save("results");
        if (!context.school.hasTeacherPassword())
            context.school.setTeacherPassword("capture-password");
        if (!context.school.loginTeacher("capture-password"))
            throw std::runtime_error("Capture login failed");
        context.role = UserRole::Teacher;
        show("home");
        save("teacher");
        show("review");
        save("review-menu");
        show("profiles");
        save("profiles-menu");
        show("bank");
        save("bank");
        click(300, 460);
        save("task-editor");
        click(400, 520);
        save("task-skills");
        click(300, 570);
        click(80, 580);
        save("answer-settings");
        click(300, 580);
        click(50, 480);
        for (int i = 0; i < 4; ++i)
            key(sf::Keyboard::Down);
        click(80, 580);
        click(150, 210);
        type("2");
        click(150, 282);
        type("3");
        click(20, 205);
        save("choice-settings");
        click(80, 580);
        click(320, 580);
        TaskVersion choice;
        for (const auto &task : context.school.tasks())
            if (task.title == "Новое задание")
                choice = task;
        context.school.publish(choice.id);
        context.selectProfile(context.school.createProfile("Выбор вариантов"), "Выбор вариантов");
        context.school.start(context.profile, SessionSettings(), {choice.id});
        context.settings = SessionSettings();
        context.current = context.school.next();
        context.role = UserRole::Student;
        show("lesson");
        click(15, 230);
        save("choice-answer");
        context.role = UserRole::Teacher;
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
        context.selectProfile("demo-check", "Демонстрация");
        show("setup");
        save("setup-1024");
        show("progress");
        save("progress-1024");
        window.setSize(sf::Vector2u(1600, 900));
        resize(1600, 900);
        context.role = UserRole::Student;
        context.selectProfile("demo-check", "Демонстрация");
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
        click(600, 300);
        save("training-support-choice");
        click(100, 590);
        save("training-support-confirm");
        sf::Event confirm = {};
        confirm.type = sf::Event::KeyPressed;
        confirm.key.code = sf::Keyboard::Enter;
        current->handle(confirm, sf::Vector2f());
        current->update();
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
            if (!navigation.empty() && !current->hasDialog() &&
                event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Tab) {
                if (navigationFocus >= 0) {
                    navigation[navigationFocus]->setFocus(false);
                    navigationFocus += event.key.shift ? -1 : 1;
                    if (navigationFocus < 0 || navigationFocus >= int(navigation.size())) {
                        navigationFocus = -1;
                        current->moveFocus(event.key.shift, false);
                    } else
                        navigation[navigationFocus]->setFocus(true);
                } else if (!current->moveFocus(event.key.shift, false)) {
                    navigationFocus = event.key.shift ? int(navigation.size()) - 1 : 0;
                    navigation[navigationFocus]->setFocus(true);
                }
                continue;
            }
            if (event.type == sf::Event::MouseButtonPressed && navigationFocus >= 0) {
                navigation[navigationFocus]->setFocus(false);
                navigationFocus = -1;
            }
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
