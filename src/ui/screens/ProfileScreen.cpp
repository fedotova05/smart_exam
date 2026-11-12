#include "HomeScreen.h"
namespace exam {
namespace ui {
void HomeScreen::manageProfiles() {
    context.school.requireTeacher();
    clear();
    title = "Ученики";
    auto active = context.school.profiles();
    auto archived = context.school.archivedProfiles();
    std::vector<std::string> names, oldNames;
    for (const auto &profile : active)
        names.push_back(profile.second);
    for (const auto &profile : archived)
        oldNames.push_back(profile.second);
    button(
        "Ученики", 0, 0, 300,
        [this] {
            profilesArchived = false;
            manageProfiles();
        },
        !profilesArchived);
    button(
        "Архив", 320, 0, 300,
        [this] {
            profilesArchived = true;
            manageProfiles();
        },
        profilesArchived);
    ListBox *students = list(profilesArchived ? oldNames : names, 0, 65, 940, 240);
    students->setRowHeight(60);
    students->setEmptyText(profilesArchived ? "Архив пуст" : "Добавьте первого ученика ниже");
    button(profilesArchived ? "Восстановить" : "В архив", 0, 325, 260,
           [this, active, archived, students] {
               if (students->selection() < 0)
                   return;
               std::string id = (profilesArchived ? archived : active)[students->selection()].first;
               if (profilesArchived) {
                   context.school.archiveProfile(id, false);
                   manageProfiles();
               } else
                   ask("Убрать профиль из списка входа? История сохранится.", [this, id] {
                       context.school.archiveProfile(id, true);
                       if (context.profile == id)
                           context.selectProfile("", "");
                       manageProfiles();
                   });
           })
        ->setEnabled(profilesArchived ? !archived.empty() : !active.empty());
    text("Добавить ученика", 0, 405, 20);
    TextField *name = field("", 0, 445, 450, 100);
    name->setPlaceholder("Имя ученика");
    auto create = button(
        "Создать", 490, 445, 230,
        [this, name] {
            context.school.createProfile(name->getText());
            manageProfiles();
        },
        true);
    name->onEnter = create->onClick;
    button("Результаты выбранного", 0, 550, 320, [this, active, archived, students] {
        if (students->selection() < 0)
            throw std::runtime_error("Выберите ученика");
        const auto &student = (profilesArchived ? archived : active)[students->selection()];
        context.selectProfile(student.first, student.second);
        context.page = "results";
    })->setEnabled(profilesArchived ? !archived.empty() : !active.empty());
    note("Архив скрывает профиль при входе. Все решения сохраняются.", 350, 561, 590);
}
} // namespace ui
} // namespace exam
