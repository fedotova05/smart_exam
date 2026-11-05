#include "application/SchoolService.h"
#include <set>
namespace exam {
void SchoolService::addTrainingContent() {
    std::set<std::string> topics;
    for (const auto &topic : storage.topics())
        topics.insert(topic.first);
    if (!topics.count("basics"))
        storage.saveTopic("basics", "Основы вычислений");
    if (!topics.count("quadratic_course"))
        storage.saveTopic("quadratic_course", "Квадратные уравнения");
    struct SkillSeed {
        std::string id, topic, name;
        std::vector<std::string> parents;
    };
    const std::vector<SkillSeed> seeds = {
        {"b_signs", "basics", "Действия со знаками", {}},
        {"b_square", "basics", "Квадрат числа", {}},
        {"b_sqrt", "basics", "Квадратный корень", {"b_square"}},
        {"b_fractions", "basics", "Вычисления с дробями", {"b_signs"}},
        {"q_coefficients", "quadratic_course", "Коэффициенты a, b, c", {}},
        {"q_discriminant", "quadratic_course", "Вычисление дискриминанта", {"b_signs", "b_square", "q_coefficients"}},
        {"q_formula", "quadratic_course", "Вычисление по формуле корней", {"b_fractions", "b_sqrt"}},
        {"q_roots", "quadratic_course", "Решение квадратного уравнения", {"q_coefficients", "q_discriminant", "q_formula"}}
    };
    std::set<std::string> skills, tasks;
    const Point positions[] = {Point(15, 300), Point(210, 300), Point(210, 420), Point(15, 420),
                               Point(405, 300), Point(405, 420), Point(15, 540), Point(405, 540)};
    for (const auto &skill : storage.skills())
        skills.insert(skill.id);
    for (const auto &task : storage.allTasks())
        tasks.insert(task.id);
    for (std::size_t i = 0; i < seeds.size(); ++i)
        if (!skills.count(seeds[i].id)) {
            storage.saveSkill(seeds[i].id, seeds[i].topic, seeds[i].name);
            storage.saveSkillPosition(seeds[i].id, positions[i]);
        }
    for (const auto &seed : seeds)
        if (!skills.count(seed.id))
            storage.setPrerequisites(seed.id, seed.parents);
    auto save = [this, &tasks](TaskVersion task) {
        if (tasks.count(task.id))
            return;
        storage.saveDraft(task);
        publish(task.id);
    };
    auto numeric = [&](const std::string &skill, const std::string &title,
                       const std::vector<std::string> &expressions, double difficulty) {
        for (std::size_t i = 0; i < expressions.size(); ++i) {
            TaskVersion task;
            task.id = "course-" + skill + "-" + std::to_string(i + 1);
            task.title = title + " · " + std::to_string(i + 1);
            task.difficulty = difficulty + i * 15;
            TaskPart part;
            part.id = "answer";
            part.kind = PartKind::Expression;
            part.prompt = "Вычислите значение. Ответ запишите числом или дробью.";
            part.expression = expressions[i];
            part.skills = {skill};
            task.parts = {part};
            save(task);
        }
    };
    numeric("b_signs", "Умножение со знаками", {"4*2*(-2)", "(-3)*5", "(-4)*(-6)", "6-(-8)", "-7-(-10)", "-2*(-3)*(-4)"}, 400);
    numeric("b_square", "Квадрат отрицательного числа", {"(-3)^2", "(-5)^2", "(-7)^2", "(-2)^2"}, 450);
    numeric("b_sqrt", "Точный квадратный корень", {"sqrt(25)", "sqrt(49)", "sqrt(81)", "sqrt(12)"}, 600);
    numeric("b_fractions", "Действия с дробями", {"-2/4", "(3-7)/6", "1/2+1/3", "(5+1)/4", "(-8)/(-12)"}, 650);
    numeric("q_formula", "Подстановка в формулу корней", {"(3+sqrt(25))/4", "(3-sqrt(25))/4", "(-2+sqrt(16))/6", "(5-sqrt(9))/4"}, 850);
    const char *equations[] = {"2*x^2-3*x-2=0", "3*x^2+2*x-1=0", "x^2-5*x+6=0", "2*x^2+x-3=0"};
    const char *coefficients[][3] = {{"2", "-3", "-2"}, {"3", "2", "-1"}, {"1", "-5", "6"}, {"2", "1", "-3"}};
    const char *discriminants[] = {"(-3)^2-4*2*(-2)", "2^2-4*3*(-1)", "(-5)^2-4*1*6", "1^2-4*2*(-3)"};
    for (int i = 0; i < 4; ++i) {
        TaskVersion task;
        task.id = "course-coefficients-" + std::to_string(i + 1);
        task.title = "Определите коэффициенты · " + std::to_string(i + 1);
        task.difficulty = 500 + i * 15;
        for (int k = 0; k < 3; ++k) {
            TaskPart part;
            part.id = std::string(1, 'a' + k);
            part.kind = PartKind::Expression;
            part.prompt = "Укажите коэффициент " + part.id + " в записи ax² + bx + c = 0. Сохраните знак числа.";
            part.expression = coefficients[i][k];
            part.displayExpression = equations[i];
            part.skills = {"q_coefficients"};
            task.parts.push_back(part);
        }
        save(task);
        task.parts.clear();
        task.id = "course-discriminant-" + std::to_string(i + 1);
        task.title = "Вычислите дискриминант · " + std::to_string(i + 1);
        task.difficulty = 750 + i * 15;
        TaskPart part;
        part.id = "answer";
        part.kind = PartKind::Expression;
        part.prompt = "Вычислите дискриминант этого уравнения по формуле D = b² − 4ac. Введите значение D.";
        part.expression = discriminants[i];
        part.displayExpression = equations[i];
        part.skills = {"q_discriminant"};
        task.parts = {part};
        save(task);
        task.id = "course-roots-" + std::to_string(i + 1);
        task.title = "Квадратное уравнение · " + std::to_string(i + 1);
        task.difficulty = 1050 + i * 15;
        part.kind = PartKind::Equation;
        part.prompt = "Решите уравнение. Все корни запишите через точку с запятой.";
        part.expression = equations[i];
        part.displayExpression.clear();
        part.skills = {"q_roots"};
        task.parts = {part};
        save(task);
    }
}
void SchoolService::addTrainingDemo() {
    for (const auto &profile : storage.profiles())
        if (profile.first == "demo-training")
            return;
    storage.saveProfile("demo-training", "Демонстрация: разбор ошибки");
    startTopics("demo-training", {"q_roots"}, true, 1);
    NextTask current = next();
    for (int i = 0; current.available && i < 16; ++i) {
        TrainingStep step = storage.trainingStep(current.attempt.id);
        bool wrong = step.stage.empty() ||
            (step.stage == "diagnosis" && (step.skill == "q_discriminant" || step.skill == "b_signs"));
        for (const auto &part : current.task.parts) {
            AnswerPayload answer;
            if (wrong)
                answer.text = "0";
            else if (part.kind == PartKind::Expression)
                answer.text = evaluateExact(parseExpression(part.expression)).text();
            else {
                Roots roots = solveEquation(part.expression);
                for (const auto &root : roots.values) {
                    if (!answer.text.empty())
                        answer.text += ";";
                    answer.text += root.text();
                }
            }
            submit(current.attempt.id, part.id, answer, true);
        }
        if (step.stage.empty())
            current = startSupport(current.attempt.id);
        else {
            finish(current.attempt.id);
            current = next();
        }
    }
}
} // namespace exam
