#include "TrainingDemoScreen.h"
#include "ui/FormulaView.h"
namespace exam { namespace ui {
void TrainingDemoScreen::refresh() {
    clear();
    title = "Как работает разбор ошибки";
    note("УЧЕБНЫЙ ПРИМЕР / КВАДРАТНЫЕ УРАВНЕНИЯ", 0, 0);
    text("Ошибка в корнях → проверка навыков → упражнения → возврат к задаче", 0, 35, 21, 940, 2);
    auto history = context.school.history("demo-training");
    std::vector<std::string> rows;
    for (std::size_t i = 0; i < history.size(); ++i)
        rows.push_back(std::to_string(i + 1) + ". " + history[i].title + (history[i].hadErrors ? " · ошибка" : " · верно"));
    if (selected >= static_cast<int>(history.size()))
        selected = 0;
    ListBox *steps = list(rows, 0, 110, 480, 360);
    steps->select(history.empty() ? -1 : selected);
    steps->onChange = [this](int index) { defer([this, index] { selected = index; refresh(); }); };
    if (!history.empty()) {
        const auto &entry = history[selected];
        text(entry.reason, 515, 110, 20, 425, 4);
        TaskVersion task = context.school.task(entry.taskId);
        const auto &part = task.parts.front();
        FormulaView *formula = add(new FormulaView(headingFace(), 425, 95), 515, 235);
        formula->setFormula(part.displayExpression.empty() ? part.expression : part.displayExpression);
        std::vector<std::string> answers;
        for (const auto &response : context.school.responses(entry.attemptId))
            answers.push_back(response.partId + ": " + response.answer.text + " · " + response.result.message);
        list(answers, 515, 345, 425, 120);
    }
    note("Это сохранённая история работы обычного алгоритма. Ошибки заданы для примера; "
         "при самостоятельном прохождении маршрут зависит от ваших ответов.", 0, 495, 940);
    button("Пройти похожий маршрут →", 0, 580, 340, [this] {
        std::string name = "Проба: квадратные уравнения";
        context.selectProfile(context.school.createProfile(name), name);
        context.role = UserRole::Student;
        context.school.startTopics(context.profile, {"q_roots"}, true, 1);
        context.settings = context.school.latestLesson(context.profile).settings;
        context.current = context.school.next();
        context.page = "lesson";
    }, true);
    button("Прогресс примера", 365, 580, 270, [this] {
        context.selectProfile("demo-training", "Демонстрация: разбор ошибки");
        context.page = "progress";
    });
    button("← В кабинет", 660, 580, 280, [this] { context.page = "home"; });
}
} }
