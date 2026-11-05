#include "../Test.h"
#include "../Fixture.h"
#include "ui/Screen.h"
#include "ui/FormulaView.h"
#include "graphics/GraphView.h"
#include "ui/screens/HomeScreen.h"
#include "ui/screens/WelcomeScreen.h"
#include "ui/screens/LessonScreen.h"
#include "ui/screens/BankScreen.h"
#include "ui/screens/KnowledgeScreen.h"
#include "ui/screens/PackageScreen.h"
#include "ui/screens/LessonSetupScreen.h"
#include "ui/screens/ProgressScreen.h"
#include "ui/screens/TrainingSetupScreen.h"
#include "io/Files.h"
#include <windows.h>
using namespace exam;
using namespace exam::ui;
namespace {
void click(Screen &screen, float x, float y) {
    sf::Event event = {};
    event.type = sf::Event::MouseButtonPressed;
    event.mouseButton.button = sf::Mouse::Left;
    screen.handle(event, sf::Vector2f(x, y));
    screen.update();
    event.type = sf::Event::MouseButtonReleased;
    screen.handle(event, sf::Vector2f(x, y));
    screen.update();
}
void key(Screen &screen, sf::Keyboard::Key code, bool control = false) {
    sf::Event event = {};
    event.type = sf::Event::KeyPressed;
    event.key.code = code;
    event.key.control = control;
    screen.handle(event, sf::Vector2f());
    screen.update();
}
void type(Screen &screen, const std::string &text) {
    for (sf::Uint32 letter : unicode(text)) {
        sf::Event event = {};
        event.type = sf::Event::TextEntered;
        event.text.unicode = letter;
        screen.handle(event, sf::Vector2f());
        screen.update();
    }
}
void capture(Screen &screen, const std::string &name) {
    sf::RenderTexture output;
    REQUIRE(output.create(1200, 800));
    output.clear(Theme::background);
    screen.setPosition(236, 100);
    output.draw(screen);
    output.display();
    REQUIRE(output.getTexture().copyToImage().saveToFile("bin/captures/" + name + ".png"));
    screen.setPosition(0, 0);
}
} // namespace
int main() {
    try {
        sf::Context graphics;
        sf::Font font;
        REQUIRE(font.loadFromFile("C:/Windows/Fonts/segoeui.ttf"));
        makeDirectories("bin/captures");
        Screen screen(font);
        TextField *input = screen.field("", 0, 0, 300);
        Button *button = screen.button("Готово", 0, 60, 150, [] {});
        click(screen, 10, 15);
        type(screen, "Ксения");
        REQUIRE(input->getText() == "Ксения");
        key(screen, sf::Keyboard::BackSpace);
        REQUIRE(input->getText() == "Ксени");
        key(screen, sf::Keyboard::Home);
        type(screen, "Я ");
        REQUIRE(input->getText() == "Я Ксени");
        key(screen, sf::Keyboard::A, true);
        type(screen, "x+1");
        REQUIRE(input->getText() == "x+1");
        key(screen, sf::Keyboard::Tab);
        REQUIRE(button->hasFocus());
        bool accepted = false;
        screen.ask("Продолжить?", [&] { accepted = true; });
        key(screen, sf::Keyboard::Enter);
        REQUIRE(accepted);
        ListBox *list = screen.list({"a", "b", "c", "d", "e", "f"}, 400, 0, 200, 80);
        sf::Event wheel = {};
        wheel.type = sf::Event::MouseWheelScrolled;
        wheel.mouseWheelScroll.delta = -1;
        screen.handle(wheel, sf::Vector2f(420, 20));
        click(screen, 420, 20);
        REQUIRE(list->selection() == 3);
        FormulaView formula(font, 800, 180);
        for (const std::string &value : {"(x+1)/(x^2+sqrt(3))", "-2;2", "none", "(x+1)^2=4"}) {
            formula.setFormula(value);
            REQUIRE(formula.errorMessage().empty());
        }
        for (const std::string &value : {"sqrt(", "x+", "1/", "x=("}) {
            formula.setFormula(value);
            REQUIRE(!formula.errorMessage().empty());
        }
        GraphView graph(font, 600, 270);
        graph.setAnswer(PartKind::Line, GraphAnswer());
        sf::Event event = {};
        event.type = sf::Event::MouseButtonPressed;
        event.mouseButton.button = sf::Mouse::Left;
        graph.handle(event, sf::Vector2f(300, 100));
        event.type = sf::Event::MouseButtonReleased;
        graph.handle(event, sf::Vector2f(300, 100));
        event.type = sf::Event::MouseButtonPressed;
        graph.handle(event, sf::Vector2f(370, 30));
        event.type = sf::Event::MouseButtonReleased;
        graph.handle(event, sf::Vector2f(370, 30));
        REQUIRE(graph.getAnswer().points.size() == 2);
        TaskPart line;
        line.kind = PartKind::Line;
        line.expression = "x+1";
        REQUIRE(GeometryChecker().check(line, graph.getAnswer()).status == CheckStatus::Correct);
        Store store(":memory:");
        prepareBank(store);
        SchoolService school(store);
        school.addStarterContent();
        AppContext context(school);
        SkillState oneAnswer;
        REQUIRE(skillEstimate(oneAnswer) == "Нет данных");
        oneAnswer.known = true;
        oneAnswer.observations = 1;
        oneAnswer.mastery = 0.5764705882;
        REQUIRE(skillEstimate(oneAnswer) == "≈ 58%");
        oneAnswer.mastery = 0.9999;
        REQUIRE(skillEstimate(oneAnswer) == "> 99%");
        REQUIRE(!canOpen(context.role, "bank"));
        WelcomeScreen welcome(font, context);
        welcome.refresh();
        capture(welcome, "entry-screen");
        click(welcome, 570, 395);
        REQUIRE(context.role == UserRole::Teacher);
        REQUIRE(context.profile.empty());
        REQUIRE(canOpen(context.role, "bank"));
        REQUIRE(!canOpen(context.role, "lesson"));
        welcome.refresh();
        click(welcome, 50, 395);
        capture(welcome, "profile-screen");
        click(welcome, 550, 170);
        type(welcome, "Мария");
        click(welcome, 560, 235);
        REQUIRE(context.role == UserRole::Student);
        REQUIRE(context.profileName == "Мария");
        REQUIRE(!canOpen(context.role, "bank"));
        REQUIRE(!canOpen(context.role, "packages"));
        REQUIRE(canOpen(context.role, "lesson"));
        REQUIRE(school.history(context.profile).empty());
        context.profile = "demo-student";
        context.profileName = "Демонстрация";
        HomeScreen home(font, context);
        home.refresh();
        capture(home, "home-screen");
        BankScreen bank(font, context);
        bank.refresh();
        REQUIRE(bank.title == "Банк заданий доступен учителю");
        context.role = UserRole::Teacher;
        bank.refresh();
        click(bank, 300, 465);
        capture(bank, "editor");
        REQUIRE(bank.title == "Редактор задания");
        // Создание составного задания, ввод ответа и возврат к части.
        TaskVersion task;
        task.id = "ui-task";
        task.title = "Корни и график функции";
        task.commonFunction = "x^2-4";
        TaskPart part;
        part.id = "a";
        part.prompt = "Найдите корни";
        part.expression = "x^2-4=0";
        part.skills = {"quadratic"};
        task.parts.push_back(part);
        part.id = "b";
        part.kind = PartKind::Parabola;
        part.prompt = "Постройте параболу";
        part.expression = "x^2-4";
        part.skills = {"parabola_graph"};
        task.parts.push_back(part);
        school.saveDraft(task);
        school.publish(task.id);
        SessionSettings settings;
        context.role = UserRole::Student;
        school.start(context.profile, settings, {task.id});
        context.current = school.next();
        LessonScreen lesson(font, context);
        lesson.refresh();
        click(lesson, 50, 340);
        type(lesson, "-2;2");
        click(lesson, 670, 95);
        REQUIRE(context.drafts["a"].text == "-2;2");
        capture(lesson, "graph");
        click(lesson, 670, 50);
        capture(lesson, "lesson");
        click(lesson, 470, 340);
        REQUIRE(school.responses(context.current.attempt.id).size() == 1);
        REQUIRE(school.responses(context.current.attempt.id)[0].result.status ==
                CheckStatus::Correct);
        click(lesson, 670, 95);
        click(lesson, 210, 520);
        type(lesson, "1");
        click(lesson, 285, 520);
        type(lesson, "0");
        click(lesson, 360, 520);
        type(lesson, "-4");
        click(lesson, 450, 520);
        REQUIRE(context.drafts["b"].graph.coefficients.size() == 3);
        capture(lesson, "graph-answer");
        click(lesson, 50, 585);
        click(lesson, 700, 585);
        key(lesson, sf::Keyboard::Enter);
        REQUIRE(context.current.attempt.finished);
        REQUIRE(school.history(context.profile)[0].result.score == 2);
        KnowledgeScreen knowledge(font, context);
        knowledge.refresh();
        capture(knowledge, "knowledge-screen");
        click(knowledge, 700, 580);
        REQUIRE(knowledge.title == "Как читать результаты");
        capture(knowledge, "progress-help");
        PackageScreen packages(font, context);
        packages.refresh();
        REQUIRE(packages.title == "Учебные пакеты доступны учителю");
        context.role = UserRole::Teacher;
        packages.refresh();
        capture(packages, "packages-screen");
        context.role = UserRole::Student;
        home.refresh();
        click(home, 60, 340);
        REQUIRE(context.page == "practice");
        school.start(context.profile, SessionSettings(), {"intro-fractions", "intro-linear", "intro-parabola"});
        home.refresh();
        click(home, 60, 340);
        REQUIRE(context.page == "lesson" && context.current.available);
        REQUIRE(context.current.task.id == "intro-fractions");
        std::string attempt = context.current.attempt.id;
        home.refresh();
        click(home, 60, 340);
        REQUIRE(context.current.attempt.id == attempt);
        // Продолжение учительского занятия восстанавливает режим проверки.
        std::string pupil = school.createProfile("Ученик с заданным режимом");
        settings.guided = true;
        settings.feedback = FeedbackPolicy::AfterTask;
        school.start(pupil, settings, {"intro-linear"});
        context.selectProfile(pupil, "Ученик с заданным режимом");
        home.refresh();
        click(home, 60, 340);
        REQUIRE(context.settings.guided);
        REQUIRE(context.settings.feedback == FeedbackPolicy::AfterTask);
        REQUIRE(context.current.task.id == "intro-linear");
        // Шаги не нужны для итогового ответа даже в старом занятии guided.
        lesson.refresh();
        click(lesson, 50, 340);
        type(lesson, "4");
        click(lesson, 470, 340);
        REQUIRE(school.responses(context.current.attempt.id).back().result.status == CheckStatus::Incomplete);
        REQUIRE(school.finish(context.current.attempt.id).score == 1);
        school.closeLesson();
        // Поле шага не заменяет итоговый ответ; выбранное действие действительно проверяется.
        settings.feedback = FeedbackPolicy::AfterStep;
        school.start(context.profile, settings, {"intro-linear"});
        context.settings = settings;
        context.current = school.next();
        lesson.refresh();
        click(lesson, 50, 340);
        type(lesson, "4");
        click(lesson, 650, 185);
        click(lesson, 650, 230);
        type(lesson, "2*x=8");
        click(lesson, 650, 320);
        click(lesson, 650, 370);
        type(lesson, "3");
        click(lesson, 770, 370);
        REQUIRE(school.responses(context.current.attempt.id).back().result.status == CheckStatus::Correct);
        REQUIRE(context.drafts["a"].text == "4");
        click(lesson, 470, 340);
        REQUIRE(school.responses(context.current.attempt.id).back().finalAnswer);
        REQUIRE(school.finish(context.current.attempt.id).score == 1);
        school.closeLesson();
        // Поиск по теме/названию и сборка занятия без записи черновиков до старта.
        context.role = UserRole::Teacher;
        LessonSetupScreen setup(font, context);
        setup.refresh();
        std::size_t beforeGeneration = school.tasks().size();
        click(setup, 280, 185);
        type(setup, "НЕСУЩЕСТВУЮЩЕЕ");
        click(setup, 490, 235);
        click(setup, 300, 550);
        REQUIRE(!setup.status.empty());
        click(setup, 280, 185);
        key(setup, sf::Keyboard::A, true);
        type(setup, "СЛОЖЕНИЕ ДРОБЕЙ");
        click(setup, 490, 235);
        click(setup, 300, 550);
        REQUIRE(setup.status.empty());
        click(setup, 230, 90);
        click(setup, 60, 490);
        REQUIRE(school.tasks().size() == beforeGeneration);
        click(setup, 680, 595);
        REQUIRE(setup.title == "Проверьте занятие");
        REQUIRE(context.settings.taskLimit == 4);
        REQUIRE(!context.settings.guided);
        click(setup, 680, 595);
        REQUIRE(setup.title == "Занятие подготовлено");
        REQUIRE(school.tasks().size() == beforeGeneration + 3);
        REQUIRE(school.latestLesson(context.profile).tasks.size() == 4);
        context.role = UserRole::Student;
        home.refresh();
        click(home, 60, 340);
        REQUIRE(context.current.available);
        REQUIRE(context.current.task.id == "intro-fractions");
        ProgressScreen progress(font, context);
        progress.refresh();
        REQUIRE(progress.title == "Прогресс по темам");
        REQUIRE(canOpen(context.role, "progress"));
        REQUIRE(canOpen(context.role, "setup"));
        context.selectProfile(school.createProfile("Темы ученика"), "Темы ученика");
        TrainingSetupScreen topics(font, context);
        topics.refresh();
        click(topics, 50, 210);
        click(topics, 340, 170);
        click(topics, 340, 210);
        click(topics, 340, 250);
        click(topics, 750, 300);
        key(topics, sf::Keyboard::A, true);
        type(topics, "1");
        click(topics, 700, 595);
        REQUIRE(context.page == "lesson");
        REQUIRE(context.settings.training);
        REQUIRE(context.settings.targetSkills == std::vector<std::string>({"q_roots"}));
        lesson.refresh();
        click(lesson, 50, 340);
        type(lesson, "0");
        click(lesson, 470, 340);
        REQUIRE(lesson.title == "Ответ пока неверный");
        click(lesson, 600, 475);
        REQUIRE(lesson.title != "Ответ пока неверный");
        click(lesson, 470, 340);
        click(lesson, 100, 475);
        REQUIRE(context.current.task.parts.front().skills == std::vector<std::string>({"q_coefficients"}));
        graphics.setActive(true);
        std::cout
            << "OK: Unicode, cursor, focus, dialog, scroll, formula, graph and student workflow\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
