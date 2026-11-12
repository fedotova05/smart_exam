#include "PackageScreen.h"
#include "ui/FileDialog.h"
#include <algorithm>
#include <sstream>
namespace exam {
namespace ui {
void PackageScreen::refresh() {
    clear();
    if (context.role != UserRole::Teacher) {
        title = "Учебные пакеты доступны учителю";
        return;
    }
    title = "Импорт и экспорт";
    text("1. Откройте файл заданий", 0, 0, 23);
    auto file = field(path, 0, 48, 680, 2048);
    file->setPlaceholder("Путь к файлу JSON");
    file->onChange = [this, file] { path = file->getText(); };
    file->onEnter = [this] { defer([this] { inspectAsync(); }); };
    button(
        "Выбрать файл", 700, 48, 240,
        [this] {
            std::string selected = choosePackage(false);
            if (!selected.empty()) {
                path = selected;
                inspectAsync();
            }
        },
        true);
    button("Открыть по пути", 0, 104, 240, [this] { inspectAsync(); });
    button("Экспорт заданий →", 700, 104, 240, [this] { exportView(); });
    if (package.items.empty()) {
        surface(0, 196, 940, 270, Theme::pale);
        text("Перенос заданий между компьютерами", 24, 222, 25, 892, 2);
        text("Импорт", 24, 288, 20);
        note("Откройте пакет и сопоставьте навыки. Задания попадут в черновики банка.", 24, 329,
             412);
        text("Экспорт", 490, 288, 20);
        note("Сохраните выбранные задания в файл. Данные учеников останутся на этом компьютере.",
             490, 329, 426);
        return;
    }
    std::vector<std::string> rows;
    for (const auto &item : package.items)
        rows.push_back(item.error.empty() ? item.task.title : "Ошибка: " + item.error);
    list(rows, 0, 171, 940, 96)->setRowHeight(48);
    text("2. Сопоставьте навыки", 0, 288, 23);
    note("Из пакета", 0, 328, 280);
    note("В вашем банке", 310, 328, 320);
    note("Связано", 660, 328, 280);
    auto source = list(package.skills, 0, 358, 280, 144);
    auto skills = context.school.skills();
    std::vector<std::string> targets, pairs;
    for (const auto &skill : skills)
        targets.push_back(skill.name);
    auto target = list(targets, 310, 358, 320, 144);
    for (const auto &pair : mapping) {
        std::string name = pair.second;
        for (const auto &skill : skills)
            if (skill.id == pair.second)
                name = skill.name;
        pairs.push_back(pair.first + " → " + name);
    }
    list(pairs, 660, 358, 280, 144)->setEmptyText("Выберите пару и нажмите «Связать»");
    button("Связать", 0, 520, 280, [this, source, target, skills] {
        if (source->selection() < 0 || target->selection() < 0)
            throw std::runtime_error("Выберите навык слева и соответствующий ему навык в центре");
        mapping[package.skills[source->selection()]] = skills[target->selection()].id;
        refresh();
    });
    button(
        "3. Добавить черновики", 660, 568, 280, [this] { importAsync(); }, true)
        ->setEnabled(mapping.size() >= package.skills.size());
    note("Связано навыков: " + std::to_string(mapping.size()) + " из " +
             std::to_string(package.skills.size()),
         0, 580, 600);
}
void PackageScreen::waiting() {
    clear();
    title = "Обработка пакета";
    text("Операция выполняется. Можно отменить её или перейти на другую страницу.", 0, 20, 22, 900,
         3);
    note("Уже сохранённые черновики остаются в банке даже после отмены.", 0, 130, 900);
    button("Отменить", 0, 230, 240, [this] {
        jobs->cancel(requestId);
        status = "Отмена запрошена";
    });
}
void PackageScreen::inspectAsync() {
    if (!jobs)
        jobs.reset(new JobQueue(context.school.databasePath()));
    leave();
    inspecting = true;
    package = PackagePreview();
    mapping.clear();
    pendingPreview.reset(new PackagePreview);
    auto output = pendingPreview;
    std::string filePath = path;
    requestId = jobs->submit(screenId, [filePath, output](Store &store, JobContext &job) {
        SchoolService school(store);
        PackageService packages(school);
        job.checkpoint();
        *output = packages.inspect(filePath);
        job.checkpoint();
        return std::string("Пакет прочитан");
    });
    waiting();
}
void PackageScreen::importAsync() {
    if (!jobs)
        jobs.reset(new JobQueue(context.school.databasePath()));
    leave();
    inspecting = false;
    auto access = context.school.teacherSession();
    auto source = package;
    auto skillMapping = mapping;
    requestId =
        jobs->submit(screenId, [source, skillMapping, access](Store &store, JobContext &job) {
            store.useTeacherAccess(access);
            SchoolService school(store);
            PackageService packages(school);
            auto report =
                packages.importPackage(source, skillMapping, [&job] { job.checkpoint(); });
            std::string text;
            for (const auto &item : report)
                text += item.id + " · " + (item.imported ? "Добавлено: " : "Пропущено: ") +
                        item.message + "\n";
            return text;
        });
    waiting();
}
void PackageScreen::leave() {
    if (jobs)
        jobs->leaveScreen(screenId);
    ++screenId;
    requestId = 0;
    pendingPreview.reset();
}
void PackageScreen::tick() {
    if (!jobs || !requestId)
        return;
    JobResult result;
    if (!jobs->poll(screenId, result) || result.id != requestId)
        return;
    requestId = 0;
    if (!result.success) {
        refresh();
        status = result.text;
        return;
    }
    if (inspecting) {
        package = *pendingPreview;
        pendingPreview.reset();
        mapping.clear();
        refresh();
        status = result.text;
    } else {
        clear();
        title = "Отчёт импорта";
        std::vector<std::string> rows;
        std::istringstream input(result.text);
        std::string row;
        while (std::getline(input, row))
            rows.push_back(row);
        list(rows, 0, 0, 940, 480);
        button("К пакетам", 0, 550, 240, [this] { refresh(); });
    }
}
void PackageScreen::exportView() {
    clear();
    title = "Экспорт заданий";
    note("Отметьте задания. Профили учеников и история решений в файл не входят.", 0, 0, 940);
    auto tasks = context.school.tasks();
    std::vector<std::string> rows;
    for (const auto &task : tasks)
        rows.push_back(task.title + (task.published ? " · опубликовано" : " · черновик"));
    auto bank = list(rows, 0, 60, 940, 420);
    bank->setRowHeight(60);
    bank->setCheckable(true);
    for (std::size_t i = 0; i < tasks.size(); ++i)
        bank->setChecked(int(i), std::find(exportIds.begin(), exportIds.end(), tasks[i].id) !=
                                     exportIds.end());
    bank->onToggle = [this, bank, tasks](int i) {
        const auto &id = tasks[i].id;
        auto found = std::find(exportIds.begin(), exportIds.end(), id);
        if (found == exportIds.end())
            exportIds.push_back(id);
        else
            exportIds.erase(found);
        bank->setChecked(i, std::find(exportIds.begin(), exportIds.end(), id) != exportIds.end());
        status = "Выбрано заданий: " + std::to_string(exportIds.size());
    };
    button(
        "Сохранить файл", 0, 550, 280,
        [this] {
            if (exportIds.empty())
                throw std::runtime_error("Отметьте хотя бы одно задание");
            std::string destination = choosePackage(true);
            if (destination.empty())
                return;
            context.packages.exportPackage(destination, exportIds);
            status = "Файл сохранён: " + destination;
        },
        true);
    button("Очистить выбор", 310, 550, 280, [this] {
        exportIds.clear();
        exportView();
    });
    button("← К импорту", 620, 550, 320, [this] { refresh(); });
}
} // namespace ui
} // namespace exam
