#include "ProgressScreen.h"
#include <algorithm>
#include <cmath>
namespace exam {
namespace ui {
void ProgressScreen::refresh() {
    clear();
    title = "Прогресс по темам";
    if (context.profile.empty()) {
        text("Выберите ученика в кабинете учителя.", 0, 20);
        button("В кабинет", 0, 100, 220, [this] { context.page = "home"; });
        return;
    }
    text(context.profileName, 0, 0, 20, 940, 1);
    auto topics = context.school.topics();
    std::vector<std::string> names = {"Все темы"};
    int selected = 0;
    for (std::size_t i = 0; i < topics.size(); ++i) {
        names.push_back(topics[i].second);
        if (topics[i].first == topic)
            selected = static_cast<int>(i + 1);
    }
    ListBox *filter = list(names, 0, 40, 270, 160);
    filter->select(selected);
    filter->onChange = [this, topics](int index) {
        defer([this, topics, index] {
            topic = index == 0 ? "" : topics[index - 1].first;
            first = 0;
            refresh();
        });
    };
    std::vector<SkillInfo> skills;
    for (const auto &skill : context.school.skills())
        if (topic.empty() || skill.topicId == topic)
            skills.push_back(skill);
    const bool hidden = context.school.progress(context.profile).hidden;
    StudentSnapshot snapshot;
    if (!hidden)
        snapshot = context.school.snapshot(context.profile);
    int known = 0, observations = 0;
    double sum = 0;
    for (const auto &skill : skills) {
        const SkillState &state = snapshot.skills[skill.id];
        if (state.known && state.observations > 0) {
            ++known;
            sum += state.mastery;
            observations += state.observations;
        }
    }
    surface(300, 40, 640, 160, Theme::pale);
    note("СРЕДНЯЯ ОЦЕНКА ИЗУЧЕННОЙ ЧАСТИ", 320, 57, 600);
    SkillState average;
    if (known > 0) {
        average.known = true;
        average.observations = observations;
        average.mastery = sum / known;
    }
    text(hidden ? "Пока скрыто" : skillEstimate(average), 320, 87, 32, 250, 1);
    text(hidden ? "Проверка после занятия"
                : "Оценено " + std::to_string(known) + " из " + std::to_string(skills.size()) +
                      " навыков",
         590, 99, 18, 330, 2);
    note("Среднее по навыкам с наблюдениями. Не является процентом освоения всей темы.", 320, 149,
         600);
    // Подсказки считаем по связанным завершённым заданиям, отдельно от наблюдений модели.
    std::map<std::string, int> hints;
    if (!hidden)
        for (const auto &entry : context.school.history(context.profile)) {
            if (!entry.result.finished)
                continue;
            const TaskVersion task = context.school.task(entry.taskId);
            for (const auto &skill : skills) {
                bool related = false;
                for (const auto &part : task.parts)
                    if (std::find(part.skills.begin(), part.skills.end(), skill.id) !=
                        part.skills.end())
                        related = true;
                if (related)
                    hints[skill.id] += entry.help;
            }
        }
    note("НАВЫК", 0, 226, 300);
    note("ОЦЕНКА", 310, 226, 300);
    note("РЕШЕНИЙ", 660, 226, 150);
    note("ПОДСКАЗОК*", 820, 226, 120);
    if (first >= static_cast<int>(skills.size()))
        first = 0;
    for (int i = first; i < std::min(first + 5, static_cast<int>(skills.size())); ++i) {
        float y = 260 + (i - first) * 50.f;
        const auto &skill = skills[i];
        const SkillState &state = snapshot.skills[skill.id];
        surface(0, y - 6, 940, 1, Theme::line);
        text(skill.name, 0, y, 16, 295, 2);
        text(hidden ? "Скрыто" : skillEstimate(state), 310, y, 18, 130, 1);
        if (!hidden && state.known && state.observations > 0)
            progressBar(static_cast<float>(state.mastery), 450, y + 12, 175);
        text(hidden ? "—" : std::to_string(state.observations), 690, y, 18, 100, 1);
        text(hidden ? "—" : std::to_string(hints[skill.id]), 850, y, 18, 90, 1);
    }
    if (skills.empty())
        note("В этой теме пока нет навыков.", 0, 270, 940);
    text("Один верный самостоятельный ответ даёт около 58%, а не 100%. Ошибки учитываются, "
         "даже после исправления. Решение с подсказкой не меняет оценку навыка.",
         0, 506, 14, 940, 2);
    note("Решений — число завершённых заданий по навыку. *Подсказки в связанных завершённых "
         "заданиях.",
         0, 545, 940);
    button("Карта связей →", 0, 586, 210, [this] { context.page = "knowledge"; });
    button("История решений", 230, 586, 230, [this] { context.page = "results"; });
    if (context.role == UserRole::Student)
        button("Тренироваться", 480, 586, 210, [this, skills] {
            context.practiceSkills.clear();
            for (const auto &skill : skills)
                context.practiceSkills.push_back(skill.id);
            context.page = "practice";
        })->setEnabled(!skills.empty());
    if (first > 0)
        button("← Назад", 710, 586, 105, [this] {
            first -= 5;
            refresh();
        });
    if (first + 5 < static_cast<int>(skills.size()))
        button("Далее →", 830, 586, 110, [this] {
            first += 5;
            refresh();
        });
}
} // namespace ui
} // namespace exam
