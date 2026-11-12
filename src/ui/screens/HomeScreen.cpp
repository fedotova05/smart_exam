#include "HomeScreen.h"
#include <algorithm>
#include "ui/FormulaView.h"
#include "ui/widgets/TextPanel.h"
namespace exam {
namespace ui {
void HomeScreen::settings() {
    context.page = context.role == UserRole::Teacher ? "setup" : "practice";
}
void HomeScreen::openLesson(const std::string &id) {
    if (context.role != UserRole::Student)
        throw std::runtime_error("Войдите как ученик");
    if (!context.school.resume(context.profile, id))
        throw std::runtime_error("Занятие уже завершено");
    selectedLesson = id;
    context.current = NextTask();
    context.drafts.clear();
    context.settings = context.school.currentLesson(context.profile).settings;
    if (context.school.lessonTime().paused)
        context.school.continueLesson();
    context.current = context.school.next();
    if (context.current.available)
        context.page = "lesson";
    else {
        refresh();
        status = context.current.reason;
    }
}
void HomeScreen::refresh() {
    if (context.role == UserRole::Teacher) {
        teacherHome();
        return;
    }
    clear();
    title = "Мои занятия";
    if (homeProfile != context.profile) {
        homeProfile = context.profile;
        selectedLesson.clear();
        showPractice = false;
        showCompleted = false;
    }
    button(
        "Задания от учителя", 0, 0, 300,
        [this] {
            showPractice = false;
            showCompleted = false;
            refresh();
        },
        !showPractice);
    button(
        "Моя практика", 320, 0, 280,
        [this] {
            showPractice = true;
            showCompleted = false;
            refresh();
        },
        showPractice);
    std::vector<SessionRecord> lessons;
    std::vector<std::string> rows;
    for (const auto &lesson : context.school.lessons(context.profile)) {
        if ((lesson.source == "practice") != showPractice || lesson.closed != showCompleted)
            continue;
        lessons.push_back(lesson);
        std::string name =
            lesson.title.empty()
                ? (lesson.tasks.empty() ? "Подбор по навыкам"
                                        : context.school.task(lesson.tasks.front()).title)
                : lesson.title;
        int done = 0;
        for (const auto &attempt : context.school.lessonAttempts(lesson.id))
            if (attempt.finished)
                ++done;
        rows.push_back(name + " · " +
                       (lesson.closed
                            ? "завершено"
                            : std::to_string(done) + "/" +
                                  std::to_string(lesson.settings.mode == SessionMode::Fixed
                                                     ? std::min(lesson.settings.taskLimit,
                                                                int(lesson.tasks.size()))
                                                     : lesson.settings.taskLimit)) +
                       (lesson.source == "legacy" ? " · ранее сохранено" : ""));
    }
    note(showCompleted  ? "ЗАВЕРШЁННЫЕ ЗАНЯТИЯ"
         : showPractice ? "ВАШИ ТРЕНИРОВКИ И ПРОВЕРКИ"
                        : "ВЫДАННЫЕ И РАНЕЕ СОХРАНЁННЫЕ ЗАНЯТИЯ",
         0, 68, 600);
    auto assignments = list(rows, 0, 106, 600, 280);
    assignments->setRowHeight(70);
    for (std::size_t i = 0; i < lessons.size(); ++i)
        if (lessons[i].id == selectedLesson)
            assignments->select(int(i));
    assignments->onChange = [this, lessons](int index) { selectedLesson = lessons[index].id; };
    assignments->onActivate = [this, lessons](int index) {
        defer([this, lessons, index] {
            if (showCompleted) {
                context.historyLesson = lessons[index].id;
                context.historyAttempt.clear();
                context.page = "results";
            } else
                openLesson(lessons[index].id);
        });
    };
    assignments->setEmptyText(
        showCompleted ? "Завершённых занятий пока нет"
        : showPractice ? "Выберите темы справа, чтобы начать тренировку"
                       : "Заданий пока нет. Учитель должен выбрать ваш профиль при назначении.");
    button(
        showCompleted ? "Открыть историю решений" : "Открыть занятие", 0, 416, 300,
        [this, lessons, assignments] {
            if (showCompleted) {
                context.historyLesson = lessons[assignments->selection()].id;
                context.historyAttempt.clear();
                context.page = "results";
                return;
            }
            if (assignments->selection() >= 0)
                openLesson(lessons[assignments->selection()].id);
        },
        true)
        ->setEnabled(!lessons.empty());
    button(showCompleted ? "К незавершённым" : "Завершённые", 320, 416, 280, [this] {
        showCompleted = !showCompleted;
        refresh();
    });
    surface(640, 0, 300, 356, Theme::pale);
    text("Самостоятельная тренировка", 660, 24, 25, 260, 3);
    note("Вы выбираете темы и число заданий.", 660, 137, 260);
    button("Выбрать темы", 660, 230, 260, [this] { context.page = "practice"; });
    note("Назначения учителя сохранятся.", 660, 296, 260);
    if (std::any_of(lessons.begin(), lessons.end(),
                    [](const SessionRecord &v) { return v.source == "legacy"; }))
        note("Ранее сохранённые занятия созданы в предыдущей версии. Их источник не был записан.",
             0, 490, 600);
    button("Мой прогресс", 0, 570, 280, [this] { context.page = "progress"; });
    button("История решений", 300, 570, 300, [this] {
        context.historyLesson.clear();
        context.page = "results";
    });
}
void HomeScreen::teacherHome() {
    clear();
    title = "Кабинет учителя";
    int published = 0, drafts = 0;
    for (const auto &task : context.school.tasks()) {
        if (task.archived)
            continue;
        if (task.published)
            ++published;
        else
            ++drafts;
    }
    text("1. Выберите ученика", 0, 0, 23);
    auto profiles = context.school.profiles();
    std::vector<std::string> names;
    int selected = -1;
    for (std::size_t i = 0; i < profiles.size(); ++i) {
        names.push_back(profiles[i].second);
        if (profiles[i].first == context.profile)
            selected = int(i);
    }
    auto students = list(names, 0, 50, 340, 240);
    students->setRowHeight(56);
    students->setEmptyText("Создайте ученика в разделе «Ученики»");
    students->select(selected);
    students->onChange = [this, profiles](int i) {
        defer([this, profiles, i] {
            context.selectProfile(profiles[i].first, profiles[i].second);
            teacherHome();
        });
    };
    button("Управление учениками", 0, 310, 340, [this] { context.page = "profiles"; });
    surface(380, 0, 560, 360, Theme::pale);
    text("2. Подготовьте занятие", 404, 24, 23);
    text(context.profile.empty() ? "Ученик не выбран" : context.profileName, 404, 77, 26, 512, 2);
    note("Добавьте готовые задания, создайте примеры в генераторе или настройте подбор по навыкам.",
         404, 157, 512);
    button(
        "Создать занятие", 404, 239, 300, [this] { settings(); }, true)
        ->setEnabled(!context.profile.empty());
    int assigned = 0;
    for (const auto &lesson : context.school.lessons(context.profile))
        if (lesson.source == "teacher" && !lesson.closed)
            ++assigned;
    note("Мои занятия → Задания от учителя. Открытых: " + std::to_string(assigned), 404, 304, 512);
    surface(0, 400, 940, 1, Theme::line);
    text("Материалы и проверка", 0, 424, 23);
    note("В банке: " + std::to_string(published) + " опубликовано, " + std::to_string(drafts) +
             " черновиков",
         0, 469, 940);
    button("Банк заданий", 0, 520, 300, [this] { context.page = "bank"; });
    button("Проверить ответы", 320, 520, 300, [this] { context.page = "review"; });
    button("Результаты ученика", 640, 520, 300, [this] {
        context.historyLesson.clear();
        context.page = "results";
    })->setEnabled(!context.profile.empty());
}
void ResultsScreen::refresh() {
    clear();
    title = "История решений";
    if (context.profile.empty()) {
        text("Выберите ученика в кабинете учителя.", 0, 20);
        button("В кабинет →", 0, 100, 230, [this] { context.page = "home"; });
        return;
    }

    LessonProgress progress = context.school.progress(context.profile, context.historyLesson);
    if (!context.historyAttempt.empty()) {
        selectedAttempt = context.historyAttempt;
        context.historyAttempt.clear();
    }
    if (!context.historyLesson.empty())
        button("Все решения", 700, -5, 240, [this] {
            context.historyLesson.clear();
            selectedAttempt.clear();
            refresh();
        });
    surface(0, 45, 940, 128, Theme::pale);
    text(progress.lesson.title.empty() ? "ВЫБРАННОЕ ЗАНЯТИЕ" : progress.lesson.title, 20, 62, 14,
         260, 1);
    heading(std::to_string(progress.finishedTasks) + " / " + std::to_string(progress.totalTasks),
            20, 98, 34);
    note("заданий завершено", 20, 146, 260);
    note("БАЛЛЫ", 300, 65, 260);
    text(lessonScore(progress), 300, 98, 32, 260, 1);
    note(progress.lesson.settings.mode == SessionMode::Dynamic ? "за выданные задания"
                                                               : "за весь набор",
         300, 146, 260);
    note(lessonNote(progress), 590, 70, 330);
    note("ВЫБЕРИТЕ ЗАДАНИЕ ИЗ ИСТОРИИ", 0, 204, 550);
    auto history = context.school.history(context.profile);
    if (!context.historyLesson.empty())
        history.erase(std::remove_if(history.begin(), history.end(),
                                     [this](const HistoryEntry &entry) {
                                         return entry.lessonId != context.historyLesson;
                                     }),
                      history.end());
    std::vector<std::string> rows;
    int index = 0;
    for (std::size_t i = 0; i < history.size(); ++i) {
        const auto &entry = history[i];
        rows.push_back(entry.title + " · " + scoreText(entry.result, entry.maximumScore));
        if (entry.attemptId == selectedAttempt)
            index = static_cast<int>(i);
    }
    ListBox *attempts = list(rows, 0, 239, 550, 320);
    attempts->select(history.empty() ? -1 : index);
    attempts->onChange = [this, history](int chosen) {
        defer([this, history, chosen] {
            selectedAttempt = history[chosen].attemptId;
            refresh();
        });
    };
    if (history.empty()) {
        text("История пока пуста", 590, 239, 24, 350, 2);
        note("После завершения первого задания здесь появятся баллы и ответы.", 590, 310, 350);
    } else {
        const HistoryEntry &entry = history[index];
        text(entry.title, 590, 204, 22, 350, 2);
        text(scoreText(entry.result, entry.maximumScore), 590, 270, 24, 350, 2);
        std::string explanation = "Подсказок: " + std::to_string(entry.help) + ". ";
        if (!entry.result.visible)
            explanation += entry.result.message;
        else if (entry.result.provisional)
            explanation += "Есть части без ответа или проверки.";
        else if (!entry.result.finished)
            explanation += "Задание ещё не завершено.";
        else if (entry.hadErrors)
            explanation += "Были ошибки — они учтены в оценке навыка.";
        else if (entry.help > 0)
            explanation += "Решение с подсказкой не повышает оценку навыка.";
        else
            explanation += "Решено самостоятельно.";
        note(explanation, 590, 344, 350);
        std::vector<std::string> responses;
        for (const auto &response : context.school.responses(entry.attemptId))
            responses.push_back(std::string(response.finalAnswer ? "Ответ: " : "Шаг: ") +
                                (response.answer.text.empty() ? "График" : response.answer.text) +
                                " · " + response.result.message);
        std::string details;
        for (const auto &row : responses)
            details += row + "\n\n";
        add(new TextPanel(font, details.empty() ? "Ответов пока нет" : details, 350, 157), 590,
            402);
    }
    button("К занятию →", 0, 580, 230, [this] { context.page = "home"; });
    button("Посмотреть навыки →", 250, 580, 280, [this] { context.page = "progress"; });
    auto repeat = context.school.reviewSkills(context.profile, context.historyLesson);
    if (!repeat.empty() && context.role == UserRole::Student)
        button("Тренировка по ошибкам →", 550, 580, 390, [this, repeat] {
            context.practiceSkills = repeat;
            context.page = "practice";
        });
}
} // namespace ui
} // namespace exam
