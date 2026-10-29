#include "Adaptation.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>
namespace exam {
namespace {
bool probability(double value) {
    return std::isfinite(value) && value > 0 && value < 1;
}
std::set<std::string> taskSkills(const TaskVersion &task) {
    std::set<std::string> skills;
    for (const TaskPart &part : task.parts)
        skills.insert(part.skills.begin(), part.skills.end());
    return skills;
}
} // namespace
Adaptation::Adaptation(const ModelParameters &settings, const SkillGraph &skills)
    : parameters(settings), graph(skills) {
    if (parameters.version != "bkt-elo-1" || !probability(parameters.initial) ||
        !probability(parameters.learn) || !probability(parameters.slip) ||
        !probability(parameters.guess) || !std::isfinite(parameters.k) || parameters.k <= 0 ||
        parameters.k > 100)
        throw std::invalid_argument("Неверные параметры модели");
}
SkillState Adaptation::update(const SkillState &before, bool correct, double difficulty) const {
    if (!std::isfinite(difficulty) || difficulty < 0 || difficulty > 3000 ||
        !std::isfinite(before.rating) || !std::isfinite(before.mastery) || before.mastery < 0 ||
        before.mastery > 1 || before.observations < 0)
        throw std::invalid_argument("Неверное состояние модели");
    const double prior = before.known ? before.mastery : parameters.initial;
    const double successKnown = correct ? 1 - parameters.slip : parameters.slip;
    const double successUnknown = correct ? parameters.guess : 1 - parameters.guess;
    const double posterior =
        prior * successKnown / (prior * successKnown + (1 - prior) * successUnknown);
    SkillState after = before;
    after.mastery = posterior + (1 - posterior) * parameters.learn;
    const double expected = 1 / (1 + std::pow(10.0, (difficulty - before.rating) / 400.0));
    after.rating = before.rating + parameters.k * ((correct ? 1.0 : 0.0) - expected);
    after.observations = before.observations + 1;
    after.known = true;
    return after;
}
std::vector<Observation> Adaptation::observe(const StudentSnapshot &before,
                                             const std::vector<PartEvidence> &evidence,
                                             double difficulty) const {
    if (before.modelVersion != parameters.version)
        throw std::invalid_argument("Версия снимка не совпадает с моделью");
    std::map<std::string, Observation> aggregated;
    for (const PartEvidence &part : evidence) {
        bool eligible = !part.usedHelp && (part.finalStatus == CheckStatus::Correct ||
                                           part.finalStatus == CheckStatus::Incorrect);
        bool correct = part.finalStatus == CheckStatus::Correct && !part.hadError;
        for (const std::string &skill : part.skills) {
            if (!graph.contains(skill))
                throw std::invalid_argument("Неизвестный навык задания");
            if (!aggregated.count(skill)) {
                Observation item;
                item.skill = skill;
                item.eligible = true;
                item.correct = true;
                auto state = before.skills.find(skill);
                if (state != before.skills.end())
                    item.before = state->second;
                aggregated[skill] = item;
            }
            Observation &item = aggregated[skill];
            item.eligible = item.eligible && eligible;
            item.correct = item.correct && correct;
        }
    }
    std::vector<Observation> result;
    for (auto &entry : aggregated) {
        Observation &item = entry.second;
        item.after = item.eligible ? update(item.before, item.correct, difficulty) : item.before;
        result.push_back(item);
    }
    return result;
}
Selection Adaptation::selectNext(const StudentSnapshot &student,
                                 const std::vector<TaskVersion> &candidates,
                                 const std::vector<std::string> &history) const {
    Selection result;
    result.reason = "Нет подходящих опубликованных заданий";
    double best = -std::numeric_limits<double>::infinity();
    std::set<std::string> lastSkills;
    if (!history.empty())
        for (const TaskVersion &task : candidates)
            if (task.id == history.back())
                lastSkills = taskSkills(task);
    for (const TaskVersion &task : candidates) {
        if (!task.published || task.archived || task.parts.empty())
            continue;
        if (!history.empty() && task.id == history.back())
            continue;
        const std::set<std::string> skills = taskSkills(task);
        if (skills.empty())
            continue;
        double priority = 0;
        int unknown = 0, weak = 0, prerequisites = 0, overlap = 0;
        bool supported = true;
        for (const std::string &skill : skills) {
            if (!graph.contains(skill)) {
                supported = false;
                break;
            }
            SkillState state;
            auto found = student.skills.find(skill);
            if (found != student.skills.end())
                state = found->second;
            if (!state.known) {
                priority += 2.0;
                ++unknown;
            } else {
                priority += 2.0 * (1 - state.mastery);
                if (state.mastery < 0.6)
                    ++weak;
            }
            priority -= std::abs(state.rating - task.difficulty) / 800.0;
            prerequisites += graph.missingPrerequisites(skill, student);
            if (lastSkills.count(skill))
                ++overlap;
        }
        if (!supported)
            continue;
        priority /= skills.size();
        priority -= 0.25 * prerequisites;
        priority -= 0.2 * overlap;
        priority -= 0.35 * std::count(history.begin(), history.end(), task.id);
        if (!result.found || priority > best + 1e-12 ||
            (std::abs(priority - best) <= 1e-12 && task.id < result.task.id)) {
            best = priority;
            result.found = true;
            result.task = task;
            result.reason = unknown ? "Диагностика неизвестного навыка"
                                    : (weak ? "Повторение недостаточно освоенного навыка"
                                            : "Закрепление освоенного навыка");
            if (prerequisites)
                result.reason += "; рекомендуется повторить предпосылки";
            if (!history.empty())
                result.reason += "; учтены сложность и разнообразие";
        }
    }
    return result;
}
} // namespace exam
