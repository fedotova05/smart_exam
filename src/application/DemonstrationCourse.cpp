#include "SchoolService.h"
namespace exam {
// Вызывается только при создании новой учебной базы.
void SchoolService::addDemonstrationCourse() {
    if (!storage.allTasks().empty())
        return;
    addStarterContent(false);
    storage.saveTopic("basics", "1. Вычисления");
    storage.saveTopic("algebra", "2. Уравнения и графики");
    storage.saveTopic("quadratic_course", "3. Квадратное уравнение по частям");
    storage.saveSkill("fractions", "basics", "Сложение дробей");
    storage.saveSkill("b_fractions", "basics", "Деление и сокращение дроби");
    storage.saveSkill("quadratic", "algebra", "Корни квадратного уравнения: закрепление");
    storage.setPrerequisites("fractions", {"b_fractions"});
    storage.setPrerequisites("linear", {"b_signs", "fractions"});
    storage.setPrerequisites("quadratic", {"q_roots"});
    storage.setPrerequisites("parabola_graph", {"quadratic", "line_graph"});
    storage.saveSkill("b_integers", "basics", "Сложение целых чисел");
    storage.setPrerequisites("b_signs", {"b_integers"});
    const std::vector<std::string> positions = {
        "b_integers", "b_signs",        "b_square",       "b_sqrt",        "b_fractions",
        "fractions",  "q_coefficients", "q_discriminant", "q_formula",     "q_roots",
        "linear",     "quadratic",      "line_graph",     "parabola_graph"};
    for (std::size_t i = 0; i < positions.size(); ++i)
        storage.saveSkillPosition(positions[i], Point(10 + (i % 3) * 195, 15 + (i / 3) * 110));
    auto save = [this](TaskVersion task) {
        auto errors = validate(task);
        if (!errors.empty())
            throw std::runtime_error(errors.front());
        storage.saveDraft(task);
        storage.publish(task.id);
    };
    for (int i = 0; i < 6; ++i) {
        TaskVersion task;
        task.id = "integer-" + std::to_string(i);
        task.title = "Целые числа · " + std::to_string(i + 1);
        task.difficulty = 200 + 15 * i;
        TaskPart part;
        part.id = "answer";
        part.kind = PartKind::Number;
        part.prompt = "Вычислите сумму. Сохраните знак результата.";
        part.expression = std::to_string(i + 2) + "-" + std::to_string(i * 2 + 5);
        part.skills = {"b_integers"};
        task.parts = {part};
        save(task);
    }
    TaskVersion choice;
    choice.id = "demo-sign-choice";
    choice.title = "Какие произведения положительные?";
    choice.difficulty = 600;
    TaskPart part;
    part.id = "answer";
    part.kind = PartKind::Choice;
    part.prompt = "Выберите все положительные произведения. Введите номера через точку с запятой.";
    part.choices = {"(-2) · (-3)", "(-2) · 3", "2 · 3", "2 · (-3)"};
    part.correctChoices = {1, 3};
    part.skills = {"b_signs"};
    choice.parts = {part};
    save(choice);
    TaskVersion polynomial;
    polynomial.id = "demo-square-polynomial";
    polynomial.title = "Раскройте квадрат суммы";
    polynomial.difficulty = 800;
    part = TaskPart();
    part.id = "answer";
    part.kind = PartKind::Polynomial;
    part.prompt = "Раскройте скобки (x + 2)². Степень вводится через ^, умножение — через *.";
    part.expression = "x^2+4*x+4";
    part.skills = {"b_square"};
    polynomial.parts = {part};
    save(polynomial);
    TaskVersion rounded;
    rounded.id = "demo-rounded-root";
    rounded.title = "Приближённый квадратный корень";
    rounded.difficulty = 850;
    part.kind = PartKind::Number;
    part.prompt = "Найдите √2. Округлите до сотых.";
    part.expression = "sqrt(2)";
    part.decimalPlaces = 2;
    part.skills = {"b_sqrt"};
    rounded.parts = {part};
    save(rounded);
    // Результат каждого примера получен через обычный сервис занятия.
    storage.saveProfile("demo-check", "Демо · проверка знаний");
    SessionSettings fixed;
    fixed.feedback = FeedbackPolicy::AfterSet;
    fixed.helpAllowed = false;
    fixed.diagnosisAllowed = false;
    fixed.taskLimit = 3;
    start("demo-check", fixed, {choice.id, polynomial.id, rounded.id});
    for (const std::string &text : {"1;3", "x^2+2*x+4", "1.41"}) {
        auto task = next();
        AnswerPayload answer;
        answer.text = text;
        submit(task.attempt.id, "answer", answer, true);
        finish(task.attempt.id);
    }
    next();
    storage.saveProfile("demo-review", "Демо · ответ для учителя");
    fixed.taskLimit = 1;
    fixed.feedback = FeedbackPolicy::AfterStep;
    start("demo-review", fixed, {rounded.id});
    auto pending = next();
    AnswerPayload uncertain;
    uncertain.text = "около полутора";
    submit(pending.attempt.id, "answer", uncertain, true);
    finish(pending.attempt.id);
    next();
    storage.saveProfile("demo-pause", "Демо · занятие на паузе");
    fixed.taskLimit = 2;
    fixed.timeLimitSeconds = 1200;
    fixed.helpAllowed = true;
    start("demo-pause", fixed, {"intro-parabola", rounded.id});
    auto paused = next();
    AnswerPayload draft;
    draft.text = "-2;2";
    storage.saveAnswerDraft(paused.attempt.id, "a", draft);
    pauseLesson();
    storage.saveProfile("student-practice", "Мой профиль · начать с нуля");
    LessonTemplate variant;
    variant.id = "demo-fixed-template";
    variant.name = "Самостоятельная: три формата ответа";
    variant.settings.taskLimit = 3;
    variant.settings.feedback = FeedbackPolicy::AfterSet;
    variant.settings.helpAllowed = false;
    variant.tasks = {choice.id, polynomial.id, rounded.id};
    storage.saveTemplate(variant);
    variant.id = "demo-adaptive-template";
    variant.name = "Подбор по истории ученика";
    variant.settings.mode = SessionMode::Dynamic;
    variant.settings.feedback = FeedbackPolicy::AfterStep;
    variant.settings.helpAllowed = true;
    variant.tasks.clear();
    storage.saveTemplate(variant);
    storage.saveProfile("demo-deep", "Демо · поиск причины ошибки");
    startTopics("demo-deep", {"q_roots"}, true, 1);
    auto current = next();
    bool failedPractice = false;
    for (int i = 0; current.available && i < 45; ++i) {
        auto step = storage.trainingStep(current.attempt.id);
        bool wrong =
            step.stage.empty() || (step.stage == "diagnosis" &&
                                   (step.skill == "q_discriminant" || step.skill == "b_signs"));
        if (step.stage == "practice" && step.skill == "b_signs" && !failedPractice) {
            wrong = true;
            failedPractice = true;
        }
        for (const auto &part : current.task.parts) {
            AnswerPayload answer;
            if (wrong)
                answer.text = "0";
            else if (part.kind == PartKind::Equation) {
                for (const auto &root : solveEquation(part.expression).values) {
                    if (!answer.text.empty())
                        answer.text += ';';
                    answer.text += root.text();
                }
            } else if (part.kind == PartKind::Choice) {
                for (int choice : part.correctChoices) {
                    if (!answer.text.empty())
                        answer.text += ';';
                    answer.text += std::to_string(choice);
                }
            } else
                answer.text = evaluateExact(parseExpression(part.expression)).text();
            submit(current.attempt.id, part.id, answer, true);
        }
        if (step.stage.empty())
            current = startSupport(current.attempt.id);
        else {
            finish(current.attempt.id);
            current = next();
        }
    }
    if (current.available)
        throw std::runtime_error("Демонстрационный маршрут не завершён");
}
} // namespace exam
