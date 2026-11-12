#include "HomeScreen.h"
#include "ui/FormulaView.h"
#include "ui/widgets/TextPanel.h"
namespace exam {
namespace ui {
void HomeScreen::settings() {
    context.page = context.role == UserRole::Teacher ? "setup" : "practice";
}
void HomeScreen::startShortLesson() {
    if (context.role != UserRole::Student)
        throw std::runtime_error("Для решения войдите как ученик");
    if (context.profile.empty())
        throw std::runtime_error("Сначала выберите профиль в настройках");
    if (context.school.resume(context.profile)) {
        context.settings = context.school.latestLesson(context.profile).settings;
        if (context.school.lessonTime().paused)
            context.school.continueLesson();
        context.current = context.school.next();
        if (context.current.available) {
            context.page = "lesson";
            return;
        }
    }
    context.page = "practice";
}
void HomeScreen::refresh() {
    if (context.role == UserRole::Teacher) {
        teacherHome();
        return;
    }
    clear();
    title = "Моё занятие";
    const LessonProgress progress = context.school.progress(context.profile);
    const bool ongoing = !progress.lesson.id.empty() && !progress.lesson.closed;
    text(context.profileName, 0, 0, 22, 940, 1);
    surface(0, 56, 580, 300, Theme::pale);
    note(ongoing ? "ТЕКУЩЕЕ ЗАНЯТИЕ" : "НОВОЕ ЗАНЯТИЕ", 24, 80, 532);
    text(ongoing ? "Продолжить с места остановки" : "Выберите, что хотите изучить", 24, 120, 27,
         532, 2);
    note(ongoing ? "Ответы сохранены. Откройте следующее незавершённое задание."
                 : "Отметьте темы и навыки. Затем выберите тренировку с подсказками или проверку "
                   "знаний.",
         24, 205, 532);
    button(
        ongoing ? "Продолжить занятие" : "Выбрать темы", 24, 286, 300,
        [this] { startShortLesson(); }, true);
    text("Результаты", 620, 65, 23);
    text(progress.lesson.id.empty() ? "Пока нет решений" : lessonScore(progress), 620, 120, 32, 320,
         2);
    note(progress.lesson.id.empty() ? "Здесь появятся баллы за последнее занятие."
                                    : lessonNote(progress),
         620, 206, 320);
    if (!progress.lesson.id.empty()) {
        progressBar(progress.totalTasks ? float(progress.finishedTasks) / progress.totalTasks : 0.f,
                    620, 280, 320);
        note(std::to_string(progress.finishedTasks) + " из " + std::to_string(progress.totalTasks) +
                 " заданий завершено",
             620, 306, 320);
    }
    text(ongoing ? "Состав занятия" : "Два режима работы", 0, 390, 22);
    if (ongoing && !progress.lesson.tasks.empty()) {
        std::vector<std::string> rows;
        for (std::size_t i = 0; i < progress.lesson.tasks.size(); ++i)
            rows.push_back(std::to_string(i + 1) + ". " +
                           context.school.task(progress.lesson.tasks[i]).title);
        auto plan = list(rows, 0, 430, 580, 118);
        plan->setRowHeight(56);
        plan->select(std::min(progress.finishedTasks, int(rows.size()) - 1));
    } else {
        text("Тренировка", 0, 438, 19);
        note("Проверка ответа, подсказки и разбор ошибок.", 0, 474, 270);
        text("Проверка знаний", 310, 438, 19);
        note("Результат после занятия, без подсказок.", 310, 474, 270);
    }
    button("Мой прогресс", 620, 430, 320, [this] { context.page = "progress"; });
    button("История решений", 620, 492, 320, [this] { context.page = "results"; });
    if (ongoing)
        button("Выбрать другие темы", 0, 570, 300, [this] { settings(); });
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
    note("Ученик найдёт подготовленное занятие в своём профиле.", 404, 304, 512);
    surface(0, 400, 940, 1, Theme::line);
    text("Материалы и проверка", 0, 424, 23);
    note("В банке: " + std::to_string(published) + " опубликовано, " + std::to_string(drafts) +
             " черновиков",
         0, 469, 940);
    button("Банк заданий", 0, 520, 300, [this] { context.page = "bank"; });
    button("Проверить ответы", 320, 520, 300, [this] { context.page = "review"; });
    button("Результаты ученика", 640, 520, 300, [this] {
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
    text(context.profileName, 0, 0, 22, 940, 1);
    LessonProgress progress = context.school.progress(context.profile);
    surface(0, 45, 940, 128, Theme::pale);
    note("ПОСЛЕДНЕЕ ЗАНЯТИЕ", 20, 65, 260);
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
        else if (entry.hadErrors)
            explanation +=
                "Были ошибки. Исправление даёт баллы, но ошибки учитываются в оценке навыка.";
        else if (entry.help > 0)
            explanation += "Решение с помощью даёт баллы, но не повышает оценку навыка.";
        else
            explanation += "Баллы относятся к этому заданию. Для оценки темы нужны другие решения.";
        note(explanation, 590, 344, 350);
        std::vector<std::string> responses;
        for (const auto &response : context.school.responses(entry.attemptId))
            responses.push_back(std::string(response.finalAnswer ? "Ответ: " : "Шаг: ") +
                                (response.answer.text.empty() ? "График" : response.answer.text) +
                                " · " + response.result.message);
        std::string details;
        for (const auto &row : responses)
            details += row + "\n\n";
        add(new TextPanel(font, details.empty() ? "Ответов пока нет" : details, 350, 107), 590,
            452);
    }
    button("К занятию →", 0, 580, 230, [this] { context.page = "home"; });
    button("Посмотреть навыки →", 250, 580, 280, [this] { context.page = "progress"; });
    auto repeat = context.school.reviewSkills(context.profile);
    if (!repeat.empty() && context.role == UserRole::Student)
        button("Тренировка по ошибкам →", 550, 580, 390, [this, repeat] {
            context.practiceSkills = repeat;
            context.page = "practice";
        });
}
} // namespace ui
} // namespace exam
