#include "TrainingSetupScreen.h"
namespace exam { namespace ui {
void TrainingSetupScreen::refresh() {
    if (profile != context.profile) {
        profile = context.profile;
        selected.clear();
        topic = "quadratic_course";
    }
    clear();
    if (!context.practiceSkills.empty()) {
        selected = std::set<std::string>(context.practiceSkills.begin(), context.practiceSkills.end());
        context.practiceSkills.clear();
        training = true;
        for (const auto &skill : context.school.skills())
            if (selected.count(skill.id)) {
                topic = skill.topicId;
                break;
            }
    }
    title = "Выберите цель занятия";
    if (profile.empty()) {
        note("Сначала выберите профиль ученика.", 0, 30);
        button("В кабинет", 0, 100, 220, [this] { context.page = "home"; });
        return;
    }
    button("Тренировка", 0, 0, 260, [this] { training = true; refresh(); }, training);
    button("Проверка знаний", 280, 0, 260, [this] { training = false; refresh(); }, !training);
    note(training ? "Проверка сразу. После ошибки можно выбрать разбор на более простых заданиях."
                  : "Набор не меняется. Оценки и рекомендации появятся после завершения занятия.", 0, 58, 940);
    auto topics = context.school.topics();
    auto all = context.school.skills();
    std::vector<std::string> names;
    int activeTopic = -1;
    for (std::size_t i = 0; i < topics.size(); ++i) {
        int total = 0, chosen = 0;
        for (const auto &skill : all)
            if (skill.topicId == topics[i].first) {
                ++total;
                if (selected.count(skill.id))
                    ++chosen;
            }
        names.push_back((chosen == 0 ? "[ ] " : chosen == total ? "[+] " : "[~] ") + topics[i].second);
        if (topics[i].first == topic)
            activeTopic = static_cast<int>(i);
    }
    text("1. Темы", 0, 110, 21);
    ListBox *topicsList = list(names, 0, 150, 280, 280);
    topicsList->select(activeTopic);
    topicsList->onChange = [this, topics, all](int index) {
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
            refresh();
        });
    };
    text("2. Навыки выбранной темы", 310, 110, 21, 360, 1);
    std::vector<SkillInfo> skills;
    names.clear();
    for (const auto &skill : all)
        if (skill.topicId == topic) {
            skills.push_back(skill);
            names.push_back((selected.count(skill.id) ? "[+] " : "[ ] ") + skill.name);
        }
    ListBox *skillList = list(names, 310, 150, 360, 280);
    skillList->select(-1);
    skillList->onChange = [this, skills](int index) {
        defer([this, skills, index] {
            if (selected.count(skills[index].id))
                selected.erase(skills[index].id);
            else
                selected.insert(skills[index].id);
            refresh();
        });
    };
    note("Нажатие добавляет или убирает тему целиком. Можно выбрать несколько тем.", 0, 450, 280);
    note("Оставьте всю тему или выберите только нужные навыки. [+] — выбран, [~] — выбрана часть темы.", 310, 450, 360);
    surface(700, 110, 1, 380, Theme::line);
    text("Ваш выбор", 730, 110, 22);
    text("Навыков: " + std::to_string(selected.size()), 730, 160, 18, 210);
    text("Основных заданий", 730, 235, 17, 210, 2);
    TextField *amount = field(count, 730, 285, 100, 2);
    amount->onChange = [this, amount] { count = amount->getText(); };
    note("От 1 до 20", 730, 340, 210);
    text(training ? "При разборе добавятся короткие упражнения на связанные навыки. После разбора занятие завершится."
                  : "Для проверки нужны разные опубликованные задания по вашему выбору.", 730, 390, 14, 210, 7);
    button("← В кабинет", 0, 580, 240, [this] { context.page = "home"; });
    button("Пример разбора", 260, 580, 260, [this] { context.page = "training-demo"; });
    button(training ? "Начать тренировку →" : "Начать проверку →", 620, 580, 320, [this] {
        double number = readNumber(count);
        if (number != static_cast<int>(number))
            throw std::runtime_error("Введите целое число заданий");
        context.school.startTopics(context.profile, std::vector<std::string>(selected.begin(), selected.end()), training, static_cast<int>(number));
        context.settings = context.school.latestLesson(context.profile).settings;
        context.current = context.school.next();
        context.page = "lesson";
    }, true);
}
} }
