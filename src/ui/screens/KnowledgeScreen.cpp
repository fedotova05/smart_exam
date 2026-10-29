#include "KnowledgeScreen.h"
#include "ui/SkillMap.h"
#include <algorithm>
namespace exam {
namespace ui {
void KnowledgeScreen::refresh() {
    clear();
    title = "Карта знаний";
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
    for (const auto &skill : skills)
        if (topic.empty() || skill.topicId == topic)
            visible.push_back(skill);
    StudentSnapshot state;
    if (!context.profile.empty())
        state = context.school.snapshot(context.profile);
    SkillMap *map = add(new SkillMap(font, visible, context.school.skillGraph(), state), 0, 94);
    map->onMove = [this](const SkillInfo &skill) {
        defer([this, skill] {
            context.school.placeSkill(skill.id, skill.position);
            selected = skill.id;
            refresh();
        });
    };
    text(context.profile.empty() ? "Профиль не выбран" : context.profileName, 630, 0, 20, 300);
    if (selected.empty() && !visible.empty())
        selected = visible.front().id;
    auto found = std::find_if(skills.begin(), skills.end(),
                              [this](const SkillInfo &skill) { return skill.id == selected; });
    if (found != skills.end()) {
        text(found->name, 630, 45, 24, 310, 3);
        text("Код: " + found->id, 630, 130, 16, 310);
        SkillState skill = state.skills[found->id];
        std::string details =
            skill.known ? "Освоение: " + std::to_string(static_cast<int>(skill.mastery * 100)) +
                              "%\nРейтинг: " + std::to_string(static_cast<int>(skill.rating)) +
                              "\nНаблюдений: " + std::to_string(skill.observations)
                        : "Нет наблюдений. Это не нулевой результат.";
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
                    history.push_back(entry.title + " · " + resultText(entry.result));
                }
            }
        details += "\nПомощь в связанных заданиях: " + std::to_string(help);
        text(details, 630, 180, 18, 310, 7);
        list(history, 630, 370, 310, 154);
    }
    button("Изменить навык", 0, 560, 220, [this] { editSkill(false); });
    button("Новый навык", 240, 560, 210, [this] { editSkill(true); });
    button("Новая тема", 470, 560, 200, [this] {
        clear();
        title = "Новая тема";
        text("Код темы", 0, 0);
        TextField *id = field("", 0, 30, 350, 80);
        text("Название", 0, 95);
        TextField *name = field("", 0, 125, 600, 120);
        button(
            "Сохранить", 0, 220, 220,
            [this, id, name] {
                context.school.saveTopic(id->getText(), name->getText());
                refresh();
            },
            true);
        button("Отмена", 240, 220, 180, [this] { refresh(); });
    });
    text("Перетащите узел. Стрелка: предпосылка → навык.", 0, 610, 14, 940, 1);
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
    text("Код", 0, 0);
    TextField *id = field(skill.id, 0, 30, 300, 80);
    id->setEnabled(create);
    text("Название", 330, 0);
    TextField *name = field(skill.name, 330, 30, 610, 120);
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
    text("Предпосылки: коды через запятую", 440, 100);
    TextField *parents =
        field(joinCodes(context.school.skillGraph()[skill.id]), 440, 130, 500, 512);
    std::vector<std::string> available;
    for (const auto &item : skills)
        available.push_back(item.id + " — " + item.name);
    ListBox *choices = list(available, 440, 190, 500, 240);
    button("Добавить выбранный", 440, 445, 270, [parents, choices, skills] {
        if (choices->selection() < 0)
            return;
        auto codes = splitCodes(parents->getText());
        const std::string &code = skills[choices->selection()].id;
        if (std::find(codes.begin(), codes.end(), code) == codes.end())
            codes.push_back(code);
        parents->setText(joinCodes(codes));
    });
    text("Для удаления связи уберите код из поля. Циклические связи не сохраняются.", 0, 340, 18,
         400, 5);
    button(
        "Сохранить", 0, 550, 220,
        [this, skill, id, name, parents, topicList, topics]() mutable {
            skill.id = id->getText();
            skill.name = name->getText();
            skill.topicId = topics[topicList->selection()].first;
            context.school.saveSkill(skill, splitCodes(parents->getText()));
            selected = skill.id;
            refresh();
        },
        true);
    button("Отмена", 240, 550, 180, [this] { refresh(); });
}
} // namespace ui
} // namespace exam
