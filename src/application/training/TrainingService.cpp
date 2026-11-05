#include "TrainingService.h"
#include "adaptation/Adaptation.h"
#include <algorithm>
#include <set>
namespace exam {
namespace {
std::set<std::string> skillsOf(const TaskVersion &task) {
    std::set<std::string> skills;
    for (const auto &part : task.parts)
        skills.insert(part.skills.begin(), part.skills.end());
    return skills;
}
}
std::string TrainingService::skillName(const std::string &id) {
    for (const auto &skill : store.skills())
        if (skill.id == id)
            return skill.name;
    return id;
}
std::vector<TaskVersion> TrainingService::scopedTasks(const std::vector<std::string> &skills) {
    std::set<std::string> allowed(skills.begin(), skills.end());
    std::vector<TaskVersion> result;
    for (const auto &task : store.publishedTasks()) {
        bool matches = !skills.empty();
        for (const auto &skill : skillsOf(task))
            if (!allowed.count(skill))
                matches = false;
        if (matches)
            result.push_back(task);
    }
    return result;
}
std::vector<TaskVersion> TrainingService::exercises(const std::string &skill,
                                                const std::vector<AttemptRecord> &history) {
    std::set<std::string> used;
    for (const auto &attempt : history)
        used.insert(attempt.taskId);
    auto tasks = scopedTasks({skill});
    tasks.erase(std::remove_if(tasks.begin(), tasks.end(), [&](const TaskVersion &task) {
        return used.count(task.id) != 0;
    }), tasks.end());
    std::sort(tasks.begin(), tasks.end(), [](const TaskVersion &a, const TaskVersion &b) {
        if (a.difficulty != b.difficulty)
            return a.difficulty < b.difficulty;
        return a.id < b.id;
    });
    return tasks;
}
bool TrainingService::successful(const AttemptRecord &attempt) {
    if (!attempt.finished || attempt.provisional || store.helpCount(attempt.id) > 0)
        return false;
    int maximum = 0;
    for (const auto &part : store.task(attempt.taskId).parts)
        maximum += part.maxScore;
    for (const auto &response : store.responses(attempt.id))
        if (response.result.status == CheckStatus::Incorrect)
            return false;
    return attempt.score == maximum;
}
std::vector<std::string> TrainingService::failedSkills(const std::string &attempt) {
    std::map<std::string, CheckStatus> last;
    for (const auto &answer : store.responses(attempt))
        if (answer.finalAnswer)
            last[answer.partId] = answer.result.status;
    std::set<std::string> result;
    for (const auto &part : store.task(store.attempt(attempt).taskId).parts)
        if (last.count(part.id) && last[part.id] == CheckStatus::Incorrect)
            result.insert(part.skills.begin(), part.skills.end());
    return std::vector<std::string>(result.begin(), result.end());
}
bool TrainingService::canSupport(const std::string &id) {
    auto attempt = store.attempt(id);
    auto lesson = store.session(attempt.sessionId);
    return lesson.settings.training && !lesson.closed &&
           store.supportOrigin(lesson.id).empty() && store.trainingStep(id).stage.empty() &&
           !failedSkills(id).empty();
}
void TrainingService::beginSupport(const std::string &id) {
    if (!canSupport(id))
        throw std::runtime_error("Разбор доступен после неверного ответа в тренировке");
    if (!store.attempt(id).finished)
        throw std::runtime_error("Сначала сохраните результат исходного задания");
    store.beginSupport(store.attempt(id).sessionId, id);
}
TrainingPlan TrainingService::plan(const std::string &session, const std::string &origin) {
    TrainingPlan result;
    auto graph = store.skillGraph();
    auto roots = failedSkills(origin);
    if (roots.empty()) {
        result.stage = "done";
        return result;
    }
    for (const auto &root : roots) {
        const auto &parents = graph[root];
        if (parents.empty())
            result.pending.push_back(root);
        else
            result.pending.insert(result.pending.end(), parents.begin(), parents.end());
    }
    std::set<std::string> checked;
    for (const auto &attempt : store.attempts(session)) {
        TrainingStep step = store.trainingStep(attempt.id);
        if (step.origin != origin || !attempt.finished)
            continue;
        bool correct = successful(attempt);
        if (step.stage == "diagnosis") {
            ++result.checks;
            checked.insert(step.skill);
            result.pending.erase(std::remove(result.pending.begin(), result.pending.end(), step.skill),
                                 result.pending.end());
            if (!correct) {
                std::vector<std::string> parents;
                for (const auto &parent : graph[step.skill])
                    if (!checked.count(parent))
                        parents.push_back(parent);
                if (parents.empty() || result.checks >= 3) {
                    result.stage = "practice";
                    result.skill = step.skill;
                } else {
                    result.bridge = step.skill;
                    result.pending.insert(result.pending.begin(), parents.begin(), parents.end());
                }
            }
        } else if (step.stage == "practice") {
            ++result.tries;
            result.successes = correct ? result.successes + 1 : 0;
            if (result.successes >= 2)
                result.stage = result.bridge.empty() ? "return" : "bridge";
            else if (result.tries >= 4) {
                result.stage = "stop";
                result.reason = "Закрепление пока вызывает трудности. Сделайте перерыв или разберите тему с учителем.";
            }
        } else if (step.stage == "bridge") {
            result.stage = correct ? "return" : "stop";
            result.reason = "Промежуточный навык пока требует объяснения. Результаты сохранены для разбора с учителем.";
        } else if (step.stage == "return") {
            int maximum = 0;
            for (const auto &part : store.task(attempt.taskId).parts)
                maximum += part.maxScore;
            // В разобранной задаче можно исправиться; самостоятельность проверит новый пример.
            correct = !attempt.provisional && attempt.score == maximum;
            result.stage = correct ? "verify" : "stop";
            result.reason = "Исходная задача пока вызывает трудности. Можно продолжить её разбор с учителем.";
        } else if (step.stage == "verify") {
            result.stage = "done";
            result.reason = correct ? "Новый похожий пример решён самостоятельно. Маршрут завершён."
                                    : "На новом примере ещё есть затруднения. Это видно в истории и прогрессе.";
        }
        if (result.stage == "diagnosis" && (result.pending.empty() || result.checks >= 3)) {
            result.stage = "practice";
            result.skill = roots.front();
        }
    }
    if (result.stage == "diagnosis")
        result.skill = result.pending.front();
    if (result.stage == "bridge")
        result.skill = result.bridge;
    if (result.stage == "return" || result.stage == "verify")
        result.skill = roots.front();
    return result;
}
NextTask TrainingService::next(const std::string &id) {
    NextTask result;
    SessionRecord lesson = store.session(id);
    auto history = store.attempts(id);
    if (lesson.closed) {
        result.reason = "Занятие завершено";
        return result;
    }
    if (!history.empty() && !history.back().finished) {
        result.available = true;
        result.attempt = history.back();
        result.task = store.task(result.attempt.taskId);
        result.reason = result.attempt.selectionReason;
        return result;
    }
    const std::string origin = store.supportOrigin(id);
    if (!origin.empty()) {
        TrainingPlan route = plan(id, origin);
        if (route.stage == "done" || route.stage == "stop") {
            store.endSupport(id);
            // После разбора показываем итог, а не начинаем следующий маршрут без объяснения.
            store.closeSession(id);
            result.reason = route.reason;
            return result;
        }
        std::string task;
        if (route.stage == "return")
            task = store.attempt(origin).taskId;
        else {
            auto candidates = exercises(route.skill, history);
            if (candidates.empty()) {
                result.reason = "В банке нет новых опубликованных упражнений на навык «" +
                                skillName(route.skill) + "». Маршрут остановлен; учитель может пополнить банк.";
                store.endSupport(id);
                store.closeSession(id);
                return result;
            }
            task = candidates.front().id;
        }
        if (store.task(task).archived) {
            result.reason = "Исходное задание перенесено в архив. История разбора сохранена.";
            store.endSupport(id);
            store.closeSession(id);
            return result;
        }
        const std::map<std::string, std::string> names = {
            {"diagnosis", "Уточняем затруднение"}, {"practice", "Закрепляем навык"},
            {"bridge", "Проверяем промежуточный навык"}, {"return", "Возвращаемся к исходной задаче"},
            {"verify", "Проверяем на новом примере"}};
        result.reason = names.at(route.stage) + ": " + skillName(route.skill);
        TrainingStep step;
        step.origin = origin;
        step.stage = route.stage;
        step.skill = route.skill;
        result.attempt = store.openTrainingAttempt(id, static_cast<int>(history.size()), task, result.reason, step);
    } else {
        std::vector<std::string> used;
        for (const auto &attempt : history)
            used.push_back(attempt.taskId);
        auto candidates = scopedTasks(lesson.settings.targetSkills);
        candidates.erase(std::remove_if(candidates.begin(), candidates.end(), [&](const TaskVersion &task) {
            return std::find(used.begin(), used.end(), task.id) != used.end();
        }), candidates.end());
        Adaptation selector(store.modelParameters(), SkillGraph(store.skillGraph()));
        Selection choice = selector.selectNext(store.snapshot(lesson.studentId), candidates, used);
        if (history.size() >= static_cast<std::size_t>(lesson.settings.taskLimit) || !choice.found) {
            result.reason = choice.found ? "План тренировки выполнен" : "Новые задания по выбранным темам закончились";
            store.closeSession(id);
            return result;
        }
        result.reason = choice.reason + "; только выбранные темы";
        result.attempt = store.openAttempt(id, static_cast<int>(history.size()), choice.task.id, result.reason);
    }
    result.available = true;
    result.task = store.task(result.attempt.taskId);
    return result;
}
} // namespace exam
