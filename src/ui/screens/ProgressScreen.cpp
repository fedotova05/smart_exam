#include "ProgressScreen.h"
#include <algorithm>
#include <cmath>
#include <set>
namespace exam {
namespace ui {
namespace {
class ResultChart : public Widget {
    std::vector<HistoryEntry> values;
    std::function<void(const HistoryEntry &)> open;
    sf::Clock animation;
    int selected = 0;
    void draw(sf::RenderTarget &target, sf::RenderStates states) const override {
        states.transform *= getTransform();
        const float baseline = 122, left = 48, width = size.x - left;
        float reveal =
            motionEnabled() ? std::min(1.f, animation.getElapsedTime().asSeconds() / .2f) : 1.f;
        reveal = 1 - (1 - reveal) * (1 - reveal) * (1 - reveal);
        for (int level : {0, 50, 100}) {
            float y = baseline - level;
            rectangle(target, states, sf::Vector2f(width, 1), Theme::line, sf::Vector2f(left, y));
            drawText(target,
                     label(font, unicode(std::to_string(level) + "%"), 12, 0, y - 5, Theme::muted),
                     states);
        }
        const float step = width / std::max(std::size_t(1), values.size());
        const float barWidth = std::min(40.f, step - 14);
        for (std::size_t i = 0; i < values.size(); ++i) {
            const auto &entry = values[i];
            int percent =
                static_cast<int>(std::round(100. * entry.result.score / entry.maximumScore));
            float x = left + step * (i + .5f) - barWidth / 2;
            sf::Color color = entry.help > 0 ? Theme::accent : Theme::ink;
            rectangle(target, states, sf::Vector2f(barWidth, std::max(2.f, percent * reveal)),
                      color, sf::Vector2f(x, baseline - std::max(2.f, percent * reveal)));
            drawText(target,
                     label(font, unicode(std::to_string(percent)), 13, x, baseline - percent - 20),
                     states);
            drawText(target,
                     label(font, unicode(std::to_string(i + 1)), 12, x + barWidth / 2 - 4,
                           baseline + 9, Theme::muted),
                     states);
            if (focused && selected == int(i))
                rectangle(target, states, sf::Vector2f(barWidth + 8, 146), sf::Color::Transparent,
                          sf::Vector2f(x - 4, 0), Theme::accent);
        }
    }

  public:
    ResultChart(const sf::Font &font, const std::vector<HistoryEntry> &entries,
                const std::function<void(const HistoryEntry &)> &action)
        : Widget(font, 556, 150), values(entries), open(action) {}
    bool handle(const sf::Event &event, sf::Vector2f mouse) override {
        if (values.empty())
            return false;
        if (event.type == sf::Event::MouseButtonReleased &&
            event.mouseButton.button == sf::Mouse::Left && contains(mouse)) {
            float x = getInverseTransform().transformPoint(mouse).x;
            if (x < 48)
                return false;
            selected =
                std::min(int(values.size()) - 1, int((x - 48) / (size.x - 48) * values.size()));
            open(values[selected]);
            return true;
        }
        if (focused && event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Left)
                selected = std::max(0, selected - 1);
            else if (event.key.code == sf::Keyboard::Right)
                selected = std::min(int(values.size()) - 1, selected + 1);
            else if (event.key.code == sf::Keyboard::Enter)
                open(values[selected]);
            else
                return false;
            return true;
        }
        return false;
    }
};
} // namespace
void ProgressScreen::refresh() {
    if (detailed)
        details();
    else
        overview();
}
void ProgressScreen::tabs() {
    button(
        "Сводка", 0, 0, 210,
        [this] {
            detailed = false;
            refresh();
        },
        !detailed);
    button(
        "По навыкам", 230, 0, 230,
        [this] {
            detailed = true;
            refresh();
        },
        detailed);
}
void ProgressScreen::overview() {
    clear();
    title = context.role == UserRole::Student ? "Мой прогресс" : "Прогресс ученика";
    if (context.profile.empty()) {
        text("Выберите ученика в кабинете учителя.", 0, 20);
        button("В кабинет", 0, 100, 220, [this] { context.page = "home"; });
        return;
    }
    tabs();
    const bool hidden = context.school.resultsHidden(context.profile);
    auto skills = context.school.skills();
    StudentSnapshot snapshot;
    if (!hidden)
        snapshot = context.school.snapshot(context.profile);
    auto history = context.school.history(context.profile);
    std::vector<HistoryEntry> checked;
    int finished = 0, independent = 0;
    for (const auto &entry : history) {
        if (!entry.result.finished)
            continue;
        ++finished;
        if (!entry.result.visible || entry.result.provisional || entry.maximumScore <= 0)
            continue;
        checked.push_back(entry);
        if (entry.help == 0 && !entry.hadErrors && entry.result.score == entry.maximumScore)
            ++independent;
    }
    std::sort(checked.begin(), checked.end(), [](const HistoryEntry &a, const HistoryEntry &b) {
        return a.completion < b.completion;
    });
    int confident = 0, repeat = 0, unseen = 0;
    SkillInfo next;
    double lowest = 2;
    std::set<std::string> supported;
    for (const auto &task : context.school.tasks())
        if (task.published && !task.archived)
            for (const auto &part : task.parts)
                supported.insert(part.skills.begin(), part.skills.end());
    for (const auto &skill : skills) {
        const auto &state = snapshot.skills[skill.id];
        if (!state.known || state.observations == 0)
            ++unseen;
        else if (state.mastery >= .75)
            ++confident;
        else {
            ++repeat;
            if (state.mastery < lowest && supported.count(skill.id)) {
                lowest = state.mastery;
                next = skill;
            }
        }
    }
    bool starting = next.id.empty();
    if (starting)
        for (const auto &skill : skills)
            if ((!snapshot.skills[skill.id].known || snapshot.skills[skill.id].observations == 0) &&
                supported.count(skill.id)) {
                next = skill;
                break;
            }
    for (int i = 0; i < 3; ++i)
        surface(i * 320.f, 64, 300, 100, i == 0 ? Theme::pale : Theme::paper);
    note("ЗАДАНИЙ ЗАВЕРШЕНО", 20, 78, 270);
    text(std::to_string(finished), 20, 104, 32, 270, 1);
    note("БЕЗ ОШИБОК И ПОДСКАЗОК", 340, 78, 270);
    text(hidden ? "—" : std::to_string(independent) + " из " + std::to_string(checked.size()), 340,
         104, 32, 270, 1);
    note("НАВЫКОВ ПРОВЕРЕНО", 660, 78, 270);
    text(hidden ? "—" : std::to_string(confident + repeat) + " из " + std::to_string(skills.size()),
         660, 104, 32, 270, 1);
    surface(0, 188, 600, 248, Theme::paper);
    text("Последние результаты", 20, 207, 22, 300, 1);
    if (!hidden && !checked.empty()) {
        note("Без подсказок", 328, 214, 130, Theme::ink);
        note("С подсказкой", 470, 214, 115, Theme::accent);
    }
    if (hidden || checked.empty()) {
        text(hidden ? "Результаты после завершения занятия" : "Здесь появятся ваши решения", 24,
             272, 23, 540, 2);
        note(hidden ? "Завершите проверку знаний, чтобы увидеть баллы и навыки."
                    : "Закончите задание. После проверки покажем баллы на графике.",
             24, 350, 540);
    } else {
        if (checked.size() > 8)
            checked.erase(checked.begin(), checked.end() - 8);
        add(new ResultChart(font, checked,
                            [this](const HistoryEntry &entry) {
                                defer([this, entry] {
                                    context.historyLesson = entry.lessonId;
                                    context.historyAttempt = entry.attemptId;
                                    context.page = "results";
                                });
                            }),
            20, 249);
        note("Баллы, % · слева старее · нажмите столбец для ответа", 20, 414, 560);
    }
    surface(620, 188, 320, 248, Theme::pale);
    note(hidden ? "ПРОДОЛЖИТЬ" : starting ? "СЛЕДУЮЩИЙ ШАГ" : "СТОИТ ПОВТОРИТЬ", 640, 208, 280);
    text(hidden            ? "Завершите занятие"
         : next.id.empty() ? "Можно идти дальше"
                           : next.name,
         640, 246, 24, 280, 3);
    note(hidden ? "Все задания доступны в «Моих занятиях»."
         : starting ? "Попробуйте новый навык в короткой тренировке."
                    : "Потренируйте этот навык с разбором ошибок.",
         640, 332, 280);
    button(
        hidden                              ? "К занятиям"
        : context.role == UserRole::Teacher ? "Подготовить занятие"
                                            : "Тренировать навык",
        640, 382, 280,
        [this, next, hidden] {
            if (hidden)
                context.page = "home";
            else if (context.role == UserRole::Teacher)
                context.page = "setup";
            else {
                context.practiceSkills.clear();
                if (!next.id.empty())
                    context.practiceSkills.push_back(next.id);
                context.page = "practice";
            }
        },
        true);
    text("Что уже получается", 0, 468, 22, 940, 1);
    if (hidden)
        note("Оценка навыков появится после проверки.", 0, 512, 940);
    else {
        const float total = std::max(std::size_t(1), skills.size());
        surface(0, 509, 940, 14, Theme::line);
        surface(0, 509, 940 * confident / total, 14, Theme::ink);
        surface(940 * confident / total, 509, 940 * repeat / total, 14, Theme::accent);
        note("Уверенно: " + std::to_string(confident) + " (от 75%)", 0, 537, 300, Theme::ink);
        note("Повторить: " + std::to_string(repeat), 320, 537, 300, Theme::accent);
        note("Ещё не проверено: " + std::to_string(unseen), 640, 537, 300);
    }
    button("История решений", 0, 586, 280, [this] {
        context.historyLesson.clear();
        context.page = "results";
    });
    button("Карта навыков", 300, 586, 280, [this] { context.page = "knowledge"; });
    button("Как читать прогресс", 640, 586, 300, [this] { explain(); });
}
void ProgressScreen::details() {
    clear();
    title = "Прогресс по навыкам";
    tabs();
    if (context.profile.empty()) {
        text("Выберите ученика в кабинете учителя.", 0, 20);
        button("В кабинет", 0, 100, 220, [this] { context.page = "home"; });
        return;
    }

    auto topics = context.school.topics();
    std::vector<std::string> names = {"Все темы"};
    int selected = 0;
    for (std::size_t i = 0; i < topics.size(); ++i) {
        names.push_back(topics[i].second);
        if (topics[i].first == topic)
            selected = static_cast<int>(i + 1);
    }
    ListBox *filter = list(names, 0, 56, 270, 144);
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
    const bool hidden = context.school.resultsHidden(context.profile);
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
    surface(300, 56, 640, 144, Theme::pale);
    note("СРЕДНЯЯ ОЦЕНКА ИЗУЧЕННОЙ ЧАСТИ", 320, 72, 600);
    SkillState average;
    if (known > 0) {
        average.known = true;
        average.observations = observations;
        average.mastery = sum / known;
    }
    text(hidden ? "Пока скрыто" : skillEstimate(average), 320, 102, 32, 250, 1);
    text(hidden ? "Проверка после занятия"
                : "Оценено " + std::to_string(known) + " из " + std::to_string(skills.size()) +
                      " навыков",
         590, 106, 18, 330, 2);
    note("Среднее по навыкам с наблюдениями. Не является процентом освоения всей темы.", 320, 164,
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
    note("ПОДСКАЗОК", 820, 226, 120);
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
    note("Оценка учитывает самостоятельные решения и ошибки.", 0, 520, 620);
    button("Как считается?", 700, 510, 240, [this] { explain(); });
    button("Карта связей →", 0, 586, 210, [this] { context.page = "knowledge"; });
    button("История решений", 230, 586, 230, [this] {
        context.historyLesson.clear();
        context.page = "results";
    });
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
void ProgressScreen::explain() {
    clear();
    title = "Как считается прогресс";
    text("Оценка навыка", 0, 20, 26);
    text("Один верный самостоятельный ответ даёт около 58%. Для уверенной оценки нужны разные "
         "решения. Ошибки учитываются и после исправления.",
         0, 80, 21, 850, 4);
    text("График и уровни", 0, 230, 26);
    text(
        "Столбец — процент баллов за одно задание. Задания различаются по сложности: это не "
        "график освоения темы. Терракотовый столбец — использована подсказка. «Уверенно» — "
        "оценка навыка от 75%. Подсказка не повышает её. Оценки до проверки на график не попадают.",
        0, 290, 21, 850, 6);
    button("← К прогрессу", 0, 550, 300, [this] { refresh(); });
}
} // namespace ui
} // namespace exam
