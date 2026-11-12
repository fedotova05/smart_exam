#include "ExamScreen.h"
#include "ui/widgets/TextPanel.h"
#include "ui/FileDialog.h"
namespace exam {
namespace ui {
void ExamScreen::refresh() {
    clear();
    title = "ОГЭ · математика · 2026";
    note("Первая часть: задания 1–19. Условия и ответы — «Решу ОГЭ». Загруженные задания доступны "
         "без интернета.",
         0, 0, 940);
    std::vector<std::string> numbers;
    for (int i = 1; i <= 19; ++i)
        numbers.push_back("Задание " + std::to_string(i));
    auto types = list(numbers, 0, 65, 220, 420);
    types->setRowHeight(50);
    types->select(number - 1);
    types->onChange = [this](int i) {
        defer([this, i] {
            number = i + 1;
            for (auto &c : ogeCategories())
                if (c.number == number) {
                    category = c.id;
                    break;
                }
            refresh();
        });
    };
    std::vector<ExamCategory> categories;
    std::vector<std::string> names;
    int active = 0;
    for (auto &c : ogeCategories())
        if (c.number == number) {
            if (c.id == category)
                active = int(names.size());
            categories.push_back(c);
            names.push_back(c.name);
        }
    auto variants = list(names, 250, 65, 400, 240);
    variants->setRowHeight(60);
    variants->select(active);
    variants->onChange = [this, categories](int i) { category = categories[i].id; };
    text("Число заданий", 690, 70, 18, 250, 1);
    auto amount = field(count, 690, 110, 120, 2);
    amount->onChange = [this, amount] { count = amount->getText(); };
    auto network = add(new Toggle(font, "Загрузить из интернета", 250), 690, 175);
    network->setValue(online);
    network->onChange = [this](bool value) { online = value; };
    note("От 1 до 20. Сначала используются сохранённые карточки, затем загружаются недостающие.",
         690, 235, 250);
    if (context.role == UserRole::Teacher) {
        text("Для ученика", 250, 335, 18);
        auto profiles = context.school.profiles();
        names.clear();
        int selected = -1;
        for (auto &p : profiles) {
            if (p.first == student)
                selected = int(names.size());
            names.push_back(p.second);
        }
        auto listProfiles = list(names, 250, 370, 400, 115);
        listProfiles->select(selected);
        listProfiles->onChange = [this, profiles](int i) { student = profiles[i].first; };
        if (selected < 0 && !profiles.empty())
            student = profiles.front().first;
    } else
        note("Навык определяется по подтипу задания. В тренировке доступны лекции и разбор на "
             "простых упражнениях.",
             250, 350, 400);
    button("Подобрать задания →", 620, 550, 320, [this] { load(); }, true);
    if (context.role == UserRole::Teacher)
        button("Импорт подборки JSON", 690, 390, 250, [this] { importFile(); });
    note("Сохранено карточек этого номера: " +
             std::to_string(context.school.examTasks(number).size()),
         0, 560, 580);
}
void ExamScreen::importFile() {
    auto path = choosePackage(false);
    if (path.empty())
        return;
    if (!jobs)
        jobs.reset(new JobQueue(context.school.databasePath()));
    leave();
    output.reset(new ExamLoadResult);
    auto target = output;
    auto access = context.school.teacherSession();
    request = jobs->submit(screen, [target, path, access](Store &store, JobContext &job) {
        store.useTeacherAccess(access);
        *target = ExamService(store).importPackage(path, [&job] { job.checkpoint(); });
        return std::string();
    });
    clear();
    title = "Импорт подборки ОГЭ";
    text("Читаем задания и связываем их с навыками.", 0, 40, 23);
    button("Отменить", 0, 160, 260, [this] { jobs->cancel(request); });
}
void ExamScreen::load() {
    double n = readNumber(count);
    if (n < 1 || n > 20 || n != int(n))
        throw std::runtime_error("Введите целое число от 1 до 20");
    if (!jobs)
        jobs.reset(new JobQueue(context.school.databasePath()));
    leave();
    output.reset(new ExamLoadResult);
    auto target = output;
    int id = category, amount = int(n);
    bool network = online;
    std::vector<std::string> excluded;
    const auto profile = context.role == UserRole::Teacher ? student : context.profile;
    if (!profile.empty())
        for (const auto &entry : context.school.history(profile))
            if (entry.result.finished)
                excluded.push_back(entry.taskId);
    request = jobs->submit(
        screen,
        [target, id, amount, network, excluded](Store &store, JobContext &job) {
            ExamService exams(store);
            *target = exams.load(id, amount, network, [&job] { job.checkpoint(); }, excluded);
            return std::string();
        },
        90);
    clear();
    title = "Подбор заданий ОГЭ";
    text("Загружаем условия, ответы и рисунки. Можно отменить загрузку или открыть другой раздел.",
         0, 40, 23, 900, 3);
    note("Уже сохранённые карточки останутся в банке.", 0, 170, 900);
    button("Отменить", 0, 250, 250, [this] {
        jobs->cancel(request);
        status = "Отмена запрошена";
    });
}
void ExamScreen::leave() {
    if (jobs)
        jobs->leaveScreen(screen);
    ++screen;
    request = 0;
    output.reset();
}
void ExamScreen::tick() {
    if (!jobs || !request)
        return;
    JobResult value;
    if (!jobs->poll(screen, value) || value.id != request)
        return;
    request = 0;
    if (!value.success) {
        refresh();
        status = value.text;
        return;
    }
    result();
}
void ExamScreen::result() {
    clear();
    title = "Задания ОГЭ готовы";
    auto ids = output->tasks;
    text("Подобрано: " + std::to_string(ids.size()) +
             ". Новых карточек: " + std::to_string(output->added),
         0, 0, 22);
    std::vector<std::string> names;
    for (auto &id : ids) {
        auto t = context.school.task(id);
        std::string skills;
        for (auto &s : context.school.skills())
            for (auto &code : t.parts[0].skills)
                if (code == s.id)
                    skills += (skills.empty() ? "" : ", ") + s.name;
        names.push_back(t.title + " · " + skills);
    }
    auto tasks = list(names, 0, 55, 940, 220);
    tasks->setRowHeight(65);
    tasks->setEmptyText("Подходящие задания не получены. Смените подтип или повторите загрузку.");
    std::string warnings;
    for (auto &w : output->warnings)
        warnings += w + "\n";
    if (ids.size() < std::size_t(readNumber(count)))
        warnings = "Найдено меньше заданий, чем запрошено. Можно начать с доступными.\n" + warnings;
    add(new TextPanel(font,
                      warnings.empty()
                          ? "Карточки сохранены. Учитель также найдёт их в банке заданий."
                          : warnings,
                      940, 140),
        0, 300);
    button("Посмотреть условие", 0, 460, 300, [this, tasks, ids] {
        if (tasks->selection() < 0)
            throw std::runtime_error("Выберите задание");
        auto t = context.school.task(ids[tasks->selection()]);
        clear();
        title = t.title;
        add(new TextPanel(font, t.description + "\n" + t.parts[0].prompt + "\n\n" + t.parameters,
                          940, 470),
            0, 0);
        button("← К подборке", 0, 540, 260, [this] { result(); });
    })->setEnabled(!ids.empty());
    button("← Назад", 0, 550, 240, [this] { refresh(); });
    bool teacher = context.role == UserRole::Teacher;
    button(
        teacher ? "Назначить ученику" : "Начать тренировку →", 620, 550, 320,
        [this, ids, teacher] {
            auto profile = teacher ? student : context.profile;
            if (profile.empty())
                throw std::runtime_error("Выберите профиль ученика");
            SessionSettings settings;
            settings.training = true;
            settings.helpAllowed = true;
            settings.taskLimit = int(ids.size());
            context.school.start(profile, settings, ids, teacher ? "teacher" : "practice",
                                 "ОГЭ-2026 · № " + std::to_string(number));
            if (teacher) {
                refresh();
                status = "Занятие назначено. Ученик увидит его в своих занятиях.";
            } else {
                context.settings = settings;
                context.drafts.clear();
                context.current = context.school.next();
                context.page = "lesson";
            }
        },
        true)
        ->setEnabled(!ids.empty());
}
} // namespace ui
} // namespace exam
