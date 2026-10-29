#include "PackageScreen.h"
#include "ui/FileDialog.h"
#include <algorithm>
namespace exam {
namespace ui {
void PackageScreen::refresh() {
    clear();
    title = "Учебные пакеты";
    text("Импорт создаёт черновики. Сопоставьте каждый навык пакета с навыком вашего банка.", 0, 0,
         18, 940, 2);
    TextField *file = field(path, 0, 58, 680, 2048);
    file->setPlaceholder("Путь к JSON-пакету");
    button("Выбрать файл", 700, 58, 240, [this] {
        std::string selected = choosePackage(false);
        if (!selected.empty()) {
            path = selected;
            package = context.packages.inspect(path);
            mapping.clear();
            refresh();
        }
    });
    button("Прочитать", 0, 112, 200, [this, file] {
        path = file->getText();
        package = context.packages.inspect(path);
        mapping.clear();
        refresh();
    });
    button("Экспорт заданий", 700, 112, 240, [this] { exportView(); });
    std::vector<std::string> rows;
    for (const auto &item : package.items)
        rows.push_back(item.sourceId + " · " + (item.error.empty() ? item.task.title : item.error));
    list(rows, 0, 180, 940, 120);
    text("Навык пакета", 0, 320, 18);
    ListBox *source = list(package.skills, 0, 350, 280, 160);
    auto skills = context.school.skills();
    std::vector<std::string> targets;
    for (const auto &skill : skills)
        targets.push_back(skill.id + " · " + skill.name);
    text("Навык в вашем банке", 310, 320, 18);
    ListBox *target = list(targets, 310, 350, 320, 160);
    text("Сопоставление", 660, 320, 18);
    std::vector<std::string> pairs;
    for (const auto &pair : mapping)
        pairs.push_back(pair.first + " → " + pair.second);
    list(pairs, 660, 350, 280, 160);
    button("Связать выбранные", 0, 550, 280, [this, source, target, skills] {
        if (source->selection() < 0 || target->selection() < 0)
            throw std::runtime_error("Выберите исходный и местный навык");
        mapping[package.skills[source->selection()]] = skills[target->selection()].id;
        refresh();
    });
    button(
        "Импортировать", 310, 550, 320,
        [this] {
            if (package.items.empty())
                throw std::runtime_error("Сначала прочитайте пакет");
            auto report = context.packages.importPackage(package, mapping);
            clear();
            title = "Отчёт импорта";
            std::vector<std::string> rows;
            for (const auto &item : report)
                rows.push_back(item.id + " · " + (item.imported ? "Добавлено: " : "Пропущено: ") +
                               item.message);
            list(rows, 0, 0, 940, 480);
            button("К пакетам", 0, 550, 240, [this] { refresh(); });
        },
        true);
}
void PackageScreen::exportView() {
    clear();
    title = "Экспорт заданий";
    text("В пакет входят только задания, коды навыков и ресурсы. Профили и история не передаются.",
         0, 0, 18, 940, 2);
    auto tasks = context.school.tasks();
    std::vector<std::string> rows;
    for (const auto &task : tasks)
        rows.push_back(task.title + " · " + task.id);
    ListBox *bank = list(rows, 0, 70, 590, 340);
    text("Выбрано: " + std::to_string(exportIds.size()), 620, 70, 20);
    list(exportIds, 620, 110, 320, 300);
    button("Добавить", 0, 435, 180, [this, bank, tasks] {
        if (bank->selection() < 0)
            return;
        std::string id = tasks[bank->selection()].id;
        if (std::find(exportIds.begin(), exportIds.end(), id) == exportIds.end())
            exportIds.push_back(id);
        exportView();
    });
    button("Очистить", 200, 435, 180, [this] {
        exportIds.clear();
        exportView();
    });
    button(
        "Сохранить JSON", 0, 550, 280,
        [this] {
            if (exportIds.empty())
                throw std::runtime_error("Выберите хотя бы одно задание");
            std::string destination = choosePackage(true);
            if (destination.empty())
                return;
            context.packages.exportPackage(destination, exportIds);
            status = "Пакет сохранён: " + destination;
        },
        true);
    button("К импорту", 310, 550, 240, [this] { refresh(); });
}
} // namespace ui
} // namespace exam
