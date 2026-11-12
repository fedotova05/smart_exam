#include "TrainingSetupScreen.h"
namespace exam {
namespace ui {
void TrainingSetupScreen::refresh() {
    if (profile != context.profile) {
        profile = context.profile;
        selected.clear();
        topic = "quadratic_course";
        focusedList = 0;
        selectedSkill = 0;
    }
    clear();
    if (!context.practiceSkills.empty()) {
        selected =
            std::set<std::string>(context.practiceSkills.begin(), context.practiceSkills.end());
        context.practiceSkills.clear();
        training = true;
        for (const auto &skill : context.school.skills())
            if (selected.count(skill.id)) {
                topic = skill.topicId;
                break;
            }
    }
    title = "Темы и режим занятия";
    if (profile.empty()) {
        note("Выберите или создайте профиль, чтобы сохранять результаты.", 0, 30);
        button("Выбрать профиль", 0, 100, 260, [this] { context.page = "welcome"; });
        return;
    }
    button(
        "Тренироваться с разбором", 0, 0, 450,
        [this] {
            training = true;
            refresh();
        },
        training);
    button(
        "Проверить свои знания", 470, 0, 470,
        [this] {
            training = false;
            refresh();
        },
        !training);
    note(training
             ? "Ответ проверяется сразу. После ошибки можно исправиться или разобрать нужный навык."
             : "Задания фиксированы. Результат появится после завершения занятия.",
         0, 58, 940);
    text("1. Откройте тему", 0, 110, 21);
    text("2. Отметьте навыки", 310, 110, 21);
    text("3. Начните занятие", 710, 110, 21, 230, 2);
    auto topics = context.school.topics();
    auto all = context.school.skills();
    std::vector<std::string> names;
    int active = -1;
    for (std::size_t i = 0; i < topics.size(); ++i) {
        names.push_back(topics[i].second);
        if (topics[i].first == topic)
            active = i;
    }
    ListBox *topicList = list(names, 0, 156, 280, 300);
    topicList->setRowHeight(60);
    topicList->setCheckable(true);
    topicList->select(active);
    for (std::size_t i = 0; i < topics.size(); ++i) {
        int total = 0, chosen = 0;
        for (const auto &skill : all)
            if (skill.topicId == topics[i].first) {
                ++total;
                if (selected.count(skill.id))
                    ++chosen;
            }
        topicList->setChecked(i, total > 0 && chosen == total);
        if (chosen > 0 && chosen < total)
            topicList->setPartial(i);
    }
    topicList->onChange = [this, topics](int index) {
        defer([this, topics, index] {
            topic = topics[index].first;
            focusedList = 1;
            selectedSkill = 0;
            refresh();
        });
    };
    topicList->onToggle = [this, topics, all](int index) {
        defer([this, topics, all, index] {
            topic = topics[index].first;
            bool full = true;
            for (const auto &skill : all)
                if (skill.topicId == topic && !selected.count(skill.id))
                    full = false;
            for (const auto &skill : all)
                if (skill.topicId == topic) {
                    if (full)
                        selected.erase(skill.id);
                    else
                        selected.insert(skill.id);
                }
            focusedList = 1;
            refresh();
        });
    };
    std::vector<SkillInfo> skills;
    names.clear();
    for (const auto &skill : all)
        if (skill.topicId == topic) {
            skills.push_back(skill);
            names.push_back(skill.name);
        }
    ListBox *skillList = list(names, 310, 156, 360, 300);
    skillList->setRowHeight(60);
    skillList->setCheckable(true);
    skillList->select(selectedSkill);
    skillList->setEmptyText("В этой теме пока нет навыков. Выберите другую тему.");
    for (std::size_t i = 0; i < skills.size(); ++i)
        skillList->setChecked(i, selected.count(skills[i].id) != 0);
    skillList->onChange = [this](int index) { selectedSkill = index; };
    skillList->onToggle = [this, skills](int index) {
        defer([this, skills, index] {
            if (selected.count(skills[index].id))
                selected.erase(skills[index].id);
            else
                selected.insert(skills[index].id);
            selectedSkill = index;
            focusedList = 2;
            refresh();
        });
    };
    note("Название открывает тему. Флажок выбирает всю тему. Стрелки — просмотр; пробел — выбор.",
         0, 475, 280);
    note("Отметьте нужные навыки. Выбор в других темах сохранится.", 310, 475, 360);
    surface(690, 110, 1, 420, Theme::line);
    text("Выбрано навыков: " + std::to_string(selected.size()), 710, 180, 18, 230, 2);
    text("Число заданий", 710, 260, 18, 230, 1);
    auto amount = field(count, 710, 295, 110, 2);
    amount->onChange = [this, amount] { count = amount->getText(); };
    note("От 1 до 20", 710, 350, 230);
    note(training ? "Разбор может добавить короткие упражнения."
                  : "Понадобятся разные опубликованные задания.",
         710, 410, 230);
    button("← К занятию", 0, 566, 240, [this] { context.page = "home"; });
    button("Как работает разбор", 260, 566, 260, [this] { context.page = "training-demo"; });
    auto start = button(
        training ? "Начать тренировку →" : "Начать проверку →", 620, 566, 320,
        [this] {
            double number = readNumber(count);
            if (number < 1 || number > 20 || number != static_cast<int>(number))
                throw std::runtime_error("Число заданий: целое от 1 до 20.");
            context.school.startTopics(context.profile,
                                       std::vector<std::string>(selected.begin(), selected.end()),
                                       training, static_cast<int>(number));
            context.settings = context.school.latestLesson(context.profile).settings;
            context.current = context.school.next();
            context.page = "lesson";
        },
        true);
    start->setEnabled(!selected.empty());
    if (selected.empty())
        status = "Отметьте хотя бы один навык или целую тему.";
    if (focusedList == 1)
        focusWidget(topicList);
    if (focusedList == 2)
        focusWidget(skillList);
}
} // namespace ui
} // namespace exam
