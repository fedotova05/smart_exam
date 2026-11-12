#include "KnowledgeScreen.h"
#include "ui/SkillMap.h"
#include <algorithm>
#include <set>
#include "io/Files.h"
namespace exam {
namespace ui {
void KnowledgeScreen::refresh() {
    clear();
    if (topic.empty() && context.profile == "demo-training")
        topic = "quadratic_course";
    title = "Карта навыков";
    auto skills = context.school.skills();
    auto topics = context.school.topics();
    std::vector<std::string> names = {"Все темы"};
    int topicIndex = 0;
    for (std::size_t i = 0; i < topics.size(); ++i) {
        names.push_back(topics[i].second);
        if (topics[i].first == topic)
            topicIndex = static_cast<int>(i + 1);
    }
    ListBox *filter = list(names, 0, 0, 600, 80);
    filter->select(topicIndex);
    filter->onChange = [this, topics](int index) {
        defer([this, topics, index] {
            topic = index == 0 ? "" : topics[index - 1].first;
            refresh();
        });
    };
    std::vector<SkillInfo> visible;
    std::set<std::string> included;
    for (const auto &skill : skills)
        if (topic.empty() || skill.topicId == topic)
            included.insert(skill.id);
    // Для выбранной темы показываем и связанные предпосылки из других тем.
    auto dependencies = context.school.skillGraph();
    std::vector<std::string> pending(included.begin(), included.end());
    for (std::size_t i = 0; i < pending.size(); ++i)
        for (const auto &parent : dependencies[pending[i]])
            if (included.insert(parent).second)
                pending.push_back(parent);
    for (const auto &skill : skills)
        if (included.count(skill.id))
            visible.push_back(skill);
    StudentSnapshot state;
    if (!context.profile.empty())
        state = context.school.snapshot(context.profile);
    const bool hidden = context.school.progress(context.profile).hidden;
    if (hidden)
        state = StudentSnapshot();
    SkillMap *map = add(new SkillMap(font, visible, context.school.skillGraph(), state), 0, 94);
    map->setEditable(context.role == UserRole::Teacher);
    map->setResultsHidden(hidden);
    map->onSelect = [this](const SkillInfo &skill) {
        defer([this, skill] {
            selected = skill.id;
            refresh();
        });
    };
    map->onMove = [this](const SkillInfo &skill) {
        defer([this, skill] {
            context.school.placeSkill(skill.id, skill.position);
            selected = skill.id;
            refresh();
        });
    };
    text(context.profile.empty() ? "Профиль не выбран" : context.profileName, 630, 0, 20, 310, 1);
    if (!visible.empty() &&
        std::find_if(visible.begin(), visible.end(), [this](const SkillInfo &skill) {
            return skill.id == selected;
        }) == visible.end())
        selected = visible.front().id;
    auto found = std::find_if(visible.begin(), visible.end(),
                              [this](const SkillInfo &skill) { return skill.id == selected; });
    if (found != visible.end()) {
        text(found->name, 630, 45, 22, 310, 2);
        SkillState skill = state.skills[found->id];
        text(hidden ? "Пока скрыто" : skillEstimate(skill), 630, 112, 32, 310, 1);
        progressBar(hidden || !skill.known ? 0.f : static_cast<float>(skill.mastery), 630, 162,
                    310);
        note("ОЦЕНКА ОСВОЕНИЯ НАВЫКА", 630, 190, 310);
        note(hidden ? "Проверка отложена до завершения занятия." : skillEvidence(skill), 630, 224,
             310);
        int help = 0;
        std::vector<std::string> history;
        if (!context.profile.empty())
            for (const auto &entry : context.school.history(context.profile)) {
                TaskVersion task = context.school.task(entry.taskId);
                bool related = false;
                for (const auto &part : task.parts)
                    if (std::find(part.skills.begin(), part.skills.end(), found->id) !=
                        part.skills.end())
                        related = true;
                if (related) {
                    help += entry.help;
                    history.push_back(entry.title + " · " +
                                      scoreText(entry.result, entry.maximumScore));
                }
            }
        note("Подсказок в связанных заданиях: " + std::to_string(help), 630, 313, 310);
        list(history, 630, 370, 310, 154);
    }
    if (context.role == UserRole::Teacher) {
        button("Изменить навык", 0, 560, 200, [this] { editSkill(false); });
        button("Новый навык", 215, 560, 200, [this] { editSkill(true); });
        button("Новая тема", 430, 560, 200, [this] {
            clear();
            title = "Новая тема";
            text("Название темы", 0, 0);
            TextField *name = field("", 0, 40, 600, 120);
            name->setPlaceholder("Например: Квадратные уравнения");
            focusWidget(name);
            button(
                "Сохранить", 0, 220, 220,
                [this, name] {
                    context.school.saveTopic(newId("topic"), name->getText());
                    refresh();
                },
                true);
            button("Отмена", 240, 220, 180, [this] { refresh(); });
        });
    }
    button("Как считается оценка?", 650, 560, 290, [this] { explain(); });
    text(context.role == UserRole::Teacher
             ? "Перетащите навык для размещения. Показаны также предпосылки из других тем."
             : "Стрелка: предпосылка → навык. Показаны необходимые навыки из других тем.",
         0, 610, 14, 940, 1);
}
void KnowledgeScreen::explain() {
    clear();
    title = "Как читать результаты";
    heading("Баллы за задание", 0, 20, 28);
    text("Баллы начисляются за последние верные ответы в частях задания. Например, 1 / 1 — полный "
         "балл за одно задание. Это не означает, что вся тема освоена.",
         0, 67, 19, 900, 3);
    surface(0, 145, 940, 1, Theme::line);
    heading("Ход занятия", 0, 174, 28);
    text("1 из 3 завершено — выполнена треть набора. Остальные задания ещё впереди. "
         "В подборе по навыкам максимум баллов известен только для уже выданных заданий.",
         0, 221, 19, 900, 3);
    surface(0, 302, 940, 1, Theme::line);
    heading("Оценка навыка", 0, 331, 28);
    text("Это прогноз по самостоятельным решениям. Первый верный ответ даёт около 58%, "
         "а не 100%. Ошибки учитываются, даже если ответ исправлен. Решение с подсказкой "
         "не меняет оценку этого навыка.",
         0, 378, 19, 900, 4);
    note("Одно наблюдение — один результат по навыку в завершённом задании, а не каждое нажатие "
         "«Проверить». Нет данных — не нулевая оценка. Прогноз не гарантирует знание темы.",
         0, 486, 900);
    button("← К навыкам", 0, 575, 230, [this] { refresh(); });
}
void KnowledgeScreen::editSkill(bool create) {
    auto skills = context.school.skills();
    SkillInfo skill;
    if (!create) {
        auto found = std::find_if(skills.begin(), skills.end(),
                                  [this](const SkillInfo &item) { return item.id == selected; });
        if (found == skills.end())
            throw std::runtime_error("Выберите навык");
        skill = *found;
    }
    auto topics = context.school.topics();
    if (topics.empty())
        throw std::runtime_error("Сначала создайте тему");
    clear();
    title = create ? "Новый навык" : "Изменение навыка";
    if (create)
        skill.id = newId("skill");
    text("Название навыка", 0, 0);
    TextField *name = field(skill.name, 0, 36, 940, 120);
    text("Тема", 0, 100);
    std::vector<std::string> rows;
    int selectedTopic = 0;
    for (std::size_t i = 0; i < topics.size(); ++i) {
        rows.push_back(topics[i].second);
        if (topics[i].first == skill.topicId)
            selectedTopic = static_cast<int>(i);
    }
    ListBox *topicList = list(rows, 0, 130, 400, 160);
    topicList->select(selectedTopic);
    text("Что нужно знать заранее", 440, 100, 20);
    std::vector<SkillInfo> available;
    std::vector<std::string> labels;
    for (const auto &item : skills)
        if (item.id != skill.id) {
            available.push_back(item);
            labels.push_back(item.name);
        }
    auto previous = context.school.skillGraph()[skill.id];
    auto parents = std::make_shared<std::set<std::string>>(previous.begin(), previous.end());
    auto choices = list(labels, 440, 140, 500, 336);
    choices->setRowHeight(56);
    choices->setCheckable(true);
    choices->setEmptyText("Других навыков пока нет");
    for (std::size_t i = 0; i < available.size(); ++i)
        choices->setChecked(int(i), parents->count(available[i].id) != 0);
    choices->onToggle = [parents, choices, available](int i) {
        if (!parents->erase(available[i].id))
            parents->insert(available[i].id);
        choices->setChecked(i, parents->count(available[i].id) != 0);
    };
    note("Отметьте навыки, на которых основан этот. Пустой список означает, что предварительные "
         "знания не требуются.",
         0, 340, 400);
    button(
        "Сохранить", 0, 550, 220,
        [this, skill, name, parents, topicList, topics]() mutable {
            skill.name = name->getText();
            skill.topicId = topics[topicList->selection()].first;
            context.school.saveSkill(skill,
                                     std::vector<std::string>(parents->begin(), parents->end()));
            selected = skill.id;
            refresh();
        },
        true);
    button("Отмена", 240, 550, 180, [this] { refresh(); });
}
} // namespace ui
} // namespace exam
