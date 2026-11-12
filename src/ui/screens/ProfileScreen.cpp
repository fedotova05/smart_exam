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
    text("Активные", 0, 0, 22);
    text("В архиве", 490, 0, 22);
    ListBox *students = list(names, 0, 45, 450, 260);
    ListBox *archive = list(oldNames, 490, 45, 450, 260);
    students->setRowHeight(52);
    archive->setRowHeight(52);
    students->setEmptyText("Добавьте первого ученика ниже");
    archive->setEmptyText("Архив пуст");
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
    button("Результаты выбранного", 0, 550, 320, [this, active, students] {
        if (students->selection() < 0)
            throw std::runtime_error("Выберите ученика");
        const auto &student = active[students->selection()];
        context.selectProfile(student.first, student.second);
        context.page = "results";
    })->setEnabled(!active.empty());
    note("Архив скрывает профиль при входе. Все решения сохраняются.", 350, 561, 590);
}
} // namespace ui
} // namespace exam
