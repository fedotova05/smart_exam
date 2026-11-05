#include "HomeScreen.h"
namespace exam {
namespace ui {
void HomeScreen::settings() {
    clear();
    title = "Начать занятие";
    auto profiles = context.school.profiles();
    std::vector<std::string> names;
    int selected = -1;
    for (std::size_t i = 0; i < profiles.size(); ++i) {
        names.push_back(profiles[i].second);
        if (profiles[i].first == context.profile)
            selected = static_cast<int>(i);
    }
    text("Профиль", 0, 0, 20);
    ListBox *students = list(names, 0, 32, 290, 160);
    students->select(selected);
    students->onChange = [this, profiles](int index) {
        if (context.profile != profiles[index].first) {
            context.current = NextTask();
            context.drafts.clear();
        }
        context.profile = profiles[index].first;
        context.profileName = profiles[index].second;
    };
    TextField *name = field("", 0, 206, 290, 60);
    name->setPlaceholder("Имя нового профиля");
    button("Создать профиль", 0, 260, 290, [this, name] {
        context.profile = context.school.createProfile(name->getText());
        context.profileName = name->getText();
        context.current = NextTask();
        context.drafts.clear();
        settings();
    });
    text("Режим", 330, 0, 20);
    ListBox *mode = list({"Фиксированный набор", "Подбор по навыкам"}, 330, 32, 290, 80);
    mode->select(static_cast<int>(context.settings.mode));
    mode->onChange = [this](int value) { context.settings.mode = static_cast<SessionMode>(value); };
    text("Показывать проверку", 650, 0, 20);
    ListBox *feedback =
        list({"После каждого шага", "После задания", "После всего набора"}, 650, 32, 290, 120);
    feedback->select(static_cast<int>(context.settings.feedback));
    feedback->onChange = [this](int value) {
        context.settings.feedback = static_cast<FeedbackPolicy>(value);
    };
    Toggle *guided = add(new Toggle(font, "Обучение по шагам", 290), 330, 130);
    guided->setValue(context.settings.guided);
    guided->onChange = [this](bool value) { context.settings.guided = value; };
    text("Число заданий (1–100)", 330, 192, 18);
    TextField *limit = field(std::to_string(context.settings.taskLimit), 330, 220, 120, 3);
    limit->onChange = [this, limit] {
        try {
            double value = readNumber(limit->getText());
            if (value >= 1 && value <= 100 && value == static_cast<int>(value))
                context.settings.taskLimit = static_cast<int>(value);
        } catch (...) {
        }
    };
    text("В шаговом режиме указывайте действие\nи новое уравнение. Итоговый ответ — отдельно.", 650,
         178, 17, 285, 4);
    auto tasks = context.school.tasks();
    std::vector<TaskVersion> published;
    std::vector<std::string> rows;
    for (const auto &task : tasks)
        if (task.published && !task.archived) {
            published.push_back(task);
            rows.push_back(task.title);
        }
    text("Задания для фиксированного набора", 0, 330, 20);
    ListBox *bank = list(rows, 0, 365, 590, 160);
    text("В наборе: " + std::to_string(fixedTasks.size()), 620, 365, 18, 320);
    button("Добавить выбранное", 620, 400, 320, [this, bank, published] {
        if (bank->selection() < 0)
            throw std::runtime_error("Сначала добавьте задания в банк");
        fixedTasks.push_back(published[bank->selection()].id);
        settings();
    });
    button("Очистить набор", 620, 454, 320, [this] {
        fixedTasks.clear();
        settings();
    });
    button(
        "Начать", 0, 554, 180,
        [this, limit] {
            if (context.profile.empty())
                throw std::runtime_error("Выберите профиль");
            double count = readNumber(limit->getText());
            if (count < 1 || count > 100 || count != static_cast<int>(count))
                throw std::runtime_error("Число заданий: целое от 1 до 100");
            context.settings.taskLimit = static_cast<int>(count);
            context.school.start(context.profile, context.settings, fixedTasks);
            context.drafts.clear();
            context.current = context.school.next();
            context.page = "lesson";
        },
        true);
    button("Продолжить", 200, 554, 180, [this] {
        if (context.profile.empty() || !context.school.resume(context.profile))
            throw std::runtime_error("Нет незавершённых занятий");
        context.current = context.school.next();
        context.drafts.clear();
        context.page = "lesson";
    });
    button("Учебные примеры", 620, 554, 320, [this] {
        context.school.addSampleBank();
        settings();
        status = "Учебные примеры добавлены в банк";
    });
    button("← На главную", 0, 602, 180, [this] { refresh(); });
}
void HomeScreen::startShortLesson() {
    if (context.profile.empty())
        throw std::runtime_error("Сначала выберите профиль в настройках");
    if (context.school.resume(context.profile)) {
        context.current = context.school.next();
        if (context.current.available) {
            context.page = "lesson";
            return;
        }
    }
    std::vector<std::string> tasks;
    for (const std::string &id : {"intro-fractions", "intro-linear", "intro-parabola"}) {
        TaskVersion task = context.school.task(id);
        if (task.published && !task.archived)
            tasks.push_back(id);
    }
    SessionSettings setup;
    setup.taskLimit = static_cast<int>(tasks.size());
    context.settings = setup;
    context.school.start(context.profile, setup, tasks);
    context.drafts.clear();
    context.current = context.school.next();
    context.page = "lesson";
}
void HomeScreen::refresh() {
    clear();
    title = "";
    note("АЛГЕБРА / КОРОТКАЯ ПРАКТИКА", 0, 0, 650);
    heading("Немного математики. В своём темпе.", 0, 35, 36);
    note("Продолжайте занятие или начните с трёх учебных заданий.", 0, 94, 650);
    surface(0, 144, 650, 236, Theme::pale);
    note("ВАШ СЛЕДУЮЩИЙ ШАГ", 28, 169, 550);
    heading("От дробей к графику функции", 28, 206, 26);
    text("Дроби, уравнение и построение графика.\nПроверка поможет разобраться с ответом.",
         28, 253, 17, 590, 2);
    button("Начать / продолжить →", 28, 320, 260, [this] { startShortLesson(); }, true);
    button("Настроить занятие", 310, 320, 240, [this] { settings(); });
    text("Как проходит занятие", 0, 418, 20);
    const char *steps[] = {"Прочитайте условие и введите ответ",
                           "Отправьте ответ на проверку",
                           "Завершите задание и перейдите дальше"};
    for (int i = 0; i < 3; ++i) {
        surface(0, 455 + i * 50.f, 650, 1, Theme::line);
        note("0" + std::to_string(i + 1), 0, 471 + i * 50.f, 40);
        text(steps[i], 45, 468 + i * 50.f, 16, 600, 1);
    }
    surface(688, 0, 408, 2, Theme::ink);
    note("ВАШ ПРОФИЛЬ", 688, 25, 408);
    text(context.profileName.empty() ? "Не выбран" : context.profileName,
         688, 60, 25, 408, 2);
    button("Профили и настройки →", 688, 125, 290, [this] { settings(); });
    surface(688, 194, 408, 1, Theme::line);
    int completed = 0;
    if (!context.profile.empty())
        for (const auto &entry : context.school.history(context.profile))
            if (entry.result.finished)
                ++completed;
    note("ВСЕГО ЗАВЕРШЕНО ЗАДАНИЙ", 688, 218, 408);
    heading(std::to_string(completed), 688, 252, 48);
    note("Посмотрите историю решений и навыки,\nкоторым стоит уделить внимание.", 688, 325, 408);
    button("Мои результаты →", 688, 390, 290, [this] { context.page = "results"; });
    button("Карта знаний →", 688, 443, 290, [this] { context.page = "knowledge"; });
    note(context.profile == "demo-student"
             ? "Демонстрация: здесь уже есть примеры решений. Для своих результатов создайте профиль."
             : "Результаты сохраняются на этом компьютере.",
         688, 525, 408);
}
void ResultsScreen::refresh() {
    clear();
    title = "Результаты";
    if (context.profile.empty()) {
        text("Выберите профиль на странице «Занятие».", 0, 20);
        return;
    }
    text(context.profileName, 0, 0, 22);
    auto history = context.school.history(context.profile);
    std::vector<std::string> rows;
    for (const auto &entry : history)
        rows.push_back(entry.title + " · " +
                       (entry.result.finished ? resultText(entry.result) : "В работе"));
    ListBox *attempts = list(rows, 0, 48, 940, 280);
    TextField *details = field("", 0, 360, 940, 2048);
    details->setEnabled(false);
    ListBox *responses = list({}, 0, 420, 940, 180);
    auto select = [this, history, details, responses](int index) {
        const HistoryEntry &entry = history[index];
        details->setText("Помощь: " + std::to_string(entry.help) + ". " + entry.reason);
        std::vector<std::string> records;
        for (const auto &response : context.school.responses(entry.attemptId))
            records.push_back(response.partId + ": " + response.answer.text + " · " +
                              response.result.message);
        responses->setItems(records);
    };
    attempts->onChange = select;
    if (!history.empty())
        select(0);
}
} // namespace ui
} // namespace exam
