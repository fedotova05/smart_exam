#include "HomeScreen.h"
namespace exam {
namespace ui {
void HomeScreen::manageProfiles() {
    context.school.requireTeacher();
    clear();
    title = "Профили учеников";
    auto active = context.school.profiles();
    auto archived = context.school.archivedProfiles();
    std::vector<std::string> names, oldNames;
    for (const auto &profile : active)
        names.push_back(profile.second);
    for (const auto &profile : archived)
        oldNames.push_back(profile.second);
    text("Активные", 0, 0, 22);
    text("В архиве", 490, 0, 22);
    ListBox *students = list(names, 0, 45, 450, 260);
    ListBox *archive = list(oldNames, 490, 45, 450, 260);
    button("В архив", 0, 325, 230, [this, active, students] {
        if (students->selection() < 0)
            throw std::runtime_error("Выберите профиль");
        std::string id = active[students->selection()].first;
        ask("Убрать профиль из списка входа? История сохранится.", [this, id] {
            context.school.archiveProfile(id, true);
            if (context.profile == id)
                context.selectProfile("", "");
            manageProfiles();
        });
    });
    button("Восстановить", 490, 325, 230, [this, archived, archive] {
        if (archive->selection() < 0)
            throw std::runtime_error("Выберите профиль в архиве");
        context.school.archiveProfile(archived[archive->selection()].first, false);
        manageProfiles();
    });
    text("Новый профиль", 0, 405, 20);
    TextField *name = field("", 0, 445, 450, 100);
    button(
        "Создать", 490, 445, 230,
        [this, name] {
            context.school.createProfile(name->getText());
            manageProfiles();
        },
        true);
    button("← Кабинет учителя", 0, 550, 300, [this] { teacherHome(); });
}
} // namespace ui
} // namespace exam
