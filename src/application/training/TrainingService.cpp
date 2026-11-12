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
} // namespace
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
    tasks.erase(std::remove_if(tasks.begin(), tasks.end(),
                               [&](const TaskVersion &task) { return used.count(task.id) != 0; }),
                tasks.end());
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
    return lesson.settings.training && lesson.settings.diagnosisAllowed && !lesson.closed &&
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
    SkillGraph checkedGraph(graph);
    auto roots = failedSkills(origin);
    if (roots.empty()) {
        result.stage = "done";
        return result;
    }
    for (const auto &root : roots) {
        if (graph[root].empty())
            result.pending.push_back(root);
        else
            result.pending.insert(result.pending.end(), graph[root].begin(), graph[root].end());
    }
    std::set<std::string> checked, expandedPractice;
    std::vector<std::string> bridges;
    int steps = 0;
    auto stop = [&](const std::string &reason) {
        result.stage = "stop";
        result.reason = reason;
    };
    auto practice = [&](const std::string &skill) {
        result.stage = "practice";
        result.skill = skill;
        result.successes = 0;
    };
    auto descend = [&](const std::string &skill) {
        if (graph[skill].empty()) {
            stop("Более простых навыков для «" + skillName(skill) +
                 "» в графе нет. Повторение остановлено: разберите правило с учителем или сделайте "
                 "перерыв.");
        } else if (!expandedPractice.insert(skill).second) {
            stop("Предпосылки этого навыка уже разобраны, но ошибка повторилась. Нужен разбор с "
                 "учителем.");
        } else {
            bridges.push_back(skill);
            result.pending = graph[skill];
            result.stage = "diagnosis";
            result.successes = 0;
            // После новой ошибки даже ранее успешную предпосылку проверяем на другом задании.
            for (const auto &parent : result.pending)
                checked.erase(parent);
        }
    };
    for (const auto &attempt : store.attempts(session)) {
        TrainingStep step = store.trainingStep(attempt.id);
        if (step.origin != origin || !attempt.finished)
            continue;
        ++steps;
        bool correct = successful(attempt);
        if (step.stage == "diagnosis") {
            checked.insert(step.skill);
            result.pending.erase(
                std::remove(result.pending.begin(), result.pending.end(), step.skill),
                result.pending.end());
            if (!correct) {
                std::vector<std::string> parents;
                for (const auto &parent : graph[step.skill])
                    if (!checked.count(parent))
                        parents.push_back(parent);
                if (parents.empty())
                    practice(step.skill);
                else {
                    bridges.push_back(step.skill);
                    result.pending = parents;
                }
            }
            if (result.stage == "diagnosis" && result.pending.empty()) {
                if (bridges.empty())
                    practice(roots.front());
                else {
                    std::string skill = bridges.back();
                    bridges.pop_back();
                    practice(skill);
                }
            }
        } else if (step.stage == "practice") {
            if (!correct)
                descend(step.skill);
            else if (++result.successes >= 2)
                result.stage = bridges.empty() ? "return" : "bridge";
        } else if (step.stage == "bridge") {
            if (!bridges.empty())
                bridges.pop_back();
            if (correct)
                result.stage = bridges.empty() ? "return" : "bridge";
            else
                descend(step.skill);
        } else if (step.stage == "return") {
            int maximum = 0;
            for (const auto &part : store.task(attempt.taskId).parts)
                maximum += part.maxScore;
            correct = !attempt.provisional && attempt.score == maximum;
            result.stage = correct ? "verify" : "stop";
            result.reason =
                "Исходная задача пока вызывает трудности. Можно продолжить её разбор с учителем.";
        } else if (step.stage == "verify") {
            result.stage = "done";
            result.reason =
                correct ? "Новый похожий пример решён самостоятельно. Маршрут завершён."
                        : "На новом примере ещё есть затруднения. Это видно в истории и прогрессе.";
        }
        if (steps >= 40 && result.stage != "done")
            stop("Разбор достиг 40 заданий. Результаты сохранены; сделайте перерыв.");
        if (result.stage == "stop" || result.stage == "done")
            break;
    }
    if (result.stage == "diagnosis")
        result.skill = result.pending.front();
    if (result.stage == "bridge")
        result.skill = bridges.back();
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
            if (route.stage == "done" && lesson.settings.mode == SessionMode::Fixed)
                return next(id);
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
                                skillName(route.skill) +
                                "». Маршрут остановлен; учитель может пополнить банк.";
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
            {"diagnosis", "Уточняем затруднение"},
            {"practice", "Закрепляем навык"},
            {"bridge", "Проверяем промежуточный навык"},
            {"return", "Возвращаемся к исходной задаче"},
            {"verify", "Проверяем на новом примере"}};
        result.reason = names.at(route.stage) + ": " + skillName(route.skill);
        TrainingStep step;
        step.origin = origin;
        step.stage = route.stage;
        step.skill = route.skill;
        result.attempt = store.openTrainingAttempt(id, static_cast<int>(history.size()), task,
                                                   result.reason, step);
    } else if (lesson.settings.mode == SessionMode::Fixed) {
        std::size_t position = 0;
        for (const auto &attempt : history)
            if (store.trainingStep(attempt.id).stage.empty())
                ++position;
        if (position >= lesson.tasks.size() || position >= std::size_t(lesson.settings.taskLimit)) {
            store.closeSession(id);
            result.reason = "Подборка выполнена";
            return result;
        }
        auto task = store.task(lesson.tasks[position]);
        result.reason = "Задание " + std::to_string(position + 1) + " из подборки";
        result.attempt = store.openAttempt(id, int(history.size()), task.id, result.reason);
    } else {
        std::vector<std::string> used;
        for (const auto &attempt : history)
            used.push_back(attempt.taskId);
        auto candidates = scopedTasks(lesson.settings.targetSkills);
        candidates.erase(std::remove_if(candidates.begin(), candidates.end(),
                                        [&](const TaskVersion &task) {
                                            return std::find(used.begin(), used.end(), task.id) !=
                                                   used.end();
                                        }),
                         candidates.end());
        Adaptation selector(store.modelParameters(), SkillGraph(store.skillGraph()));
        Selection choice = selector.selectNext(store.snapshot(lesson.studentId), candidates, used);
        if (history.size() >= static_cast<std::size_t>(lesson.settings.taskLimit) ||
            !choice.found) {
            result.reason = choice.found ? "План тренировки выполнен"
                                         : "Новые задания по выбранным темам закончились";
            store.closeSession(id);
            return result;
        }
        result.reason = choice.reason + "; только выбранные темы";
        result.attempt =
            store.openAttempt(id, static_cast<int>(history.size()), choice.task.id, result.reason);
    }
    result.available = true;
    result.task = store.task(result.attempt.taskId);
    return result;
}
} // namespace exam
