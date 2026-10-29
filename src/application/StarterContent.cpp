#include "SchoolService.h"
#include <set>
namespace exam {
void SchoolService::addStarterContent() {
    const char *ids[] = {"fractions", "linear", "quadratic", "line_graph", "parabola_graph"};
    const char *names[] = {"Действия с дробями", "Линейные уравнения", "Квадратные уравнения",
                           "График прямой", "График параболы"};
    std::set<std::string> known;
    for (const auto &skill : storage.skills())
        known.insert(skill.id);
    bool hasTopic = false;
    for (const auto &topic : storage.topics())
        if (topic.first == "algebra")
            hasTopic = true;
    if (!hasTopic)
        storage.saveTopic("algebra", "Алгебра");
    for (int i = 0; i < 5; ++i)
        if (!known.count(ids[i])) {
            storage.saveSkill(ids[i], "algebra", names[i]);
            storage.saveSkillPosition(ids[i], Point(15 + (i % 3) * 195, 25 + (i / 3) * 140));
            if (i == 2 || i == 3)
                storage.savePrerequisite(ids[i], "linear");
            if (i == 4)
                storage.savePrerequisite(ids[i], "quadratic");
        }
    const char *taskIds[] = {"intro-fractions", "intro-linear", "intro-quadratic", "intro-line",
                             "intro-parabola"};
    const char *titles[] = {"Сложение дробей", "Линейное уравнение", "Два корня уравнения",
                            "Прямая по двум точкам", "Корни и парабола"};
    const char *prompts[] = {"Вычислите значение выражения. Ответ запишите дробью.",
                             "Решите уравнение. В ответе запишите значение x.",
                             "Найдите все корни уравнения. Разделите их точкой с запятой.",
                             "Постройте прямую, отметив на ней две точки.",
                             "Найдите нули функции. Затем постройте её график во второй части."};
    const char *expressions[] = {"1/2+1/3", "2*x+3=11", "x^2-5*x+6=0", "2*x+1", "x^2-4=0"};
    std::set<std::string> existing;
    for (const auto &task : storage.allTasks())
        existing.insert(task.id);
    for (int i = 0; i < 5; ++i)
        if (!existing.count(taskIds[i])) {
            TaskVersion task;
            task.id = taskIds[i];
            task.title = titles[i];
            task.difficulty = 800 + i * 100;
            TaskPart part;
            part.id = "a";
            part.prompt = prompts[i];
            part.expression = expressions[i];
            part.kind = i == 0   ? PartKind::Expression
                        : i == 3 ? PartKind::Line
                                 : PartKind::Equation;
            part.skills = {ids[i == 4 ? 2 : i]};
            task.parts.push_back(part);
            if (i == 4) {
                task.description = "Исследуем функцию y = x² − 4.";
                task.commonFunction = "x^2-4";
                part.id = "b";
                part.kind = PartKind::Parabola;
                part.prompt = "Постройте график y = x² − 4.";
                part.expression = "x^2-4";
                part.skills = {"parabola_graph"};
                part.maxScore = 2;
                task.parts.push_back(part);
            }
            storage.saveDraft(task);
            publish(task.id);
        }
    for (const auto &profile : storage.profiles())
        if (profile.first == "demo-student")
            return;
    storage.saveProfile("demo-student", "Демонстрация");
    // Пример истории проходит через те же проверки, что и обычное занятие.
    Adaptation model(storage.modelParameters(), SkillGraph(storage.skillGraph()));
    SessionService lesson(storage, checker, &model, &model);
    SessionRecord record;
    record.id = "intro-history";
    record.studentId = "demo-student";
    record.settings.taskLimit = 2;
    record.tasks = {"intro-fractions", "intro-linear"};
    lesson.start(record);
    for (int i = 0; i < 2; ++i) {
        NextTask task = lesson.next(record.id);
        ResponseRecord response;
        response.attemptId = task.attempt.id;
        response.partId = "a";
        if (i == 1) {
            response.requestId = "intro-wrong";
            response.answer.text = "3";
            lesson.submitResponse(response);
        }
        response.requestId = "intro-answer-" + std::to_string(i);
        response.answer.text = i == 0 ? "5/6" : "4";
        lesson.submitResponse(response);
        lesson.finishAttempt(task.attempt.id);
    }
    lesson.next(record.id);
}
} // namespace exam
