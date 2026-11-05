#include "HomeScreen.h"
#include "ui/FormulaView.h"
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
    title = "";
    const LessonProgress progress = context.school.progress(context.profile);
    bool ongoing = !progress.lesson.id.empty() && !progress.lesson.closed;
    std::vector<std::string> taskIds;
    int position = 0;
    int total = 0;
    if (ongoing) {
        position = progress.finishedTasks;
        total = progress.totalTasks;
        taskIds = progress.lesson.tasks;
    }
    std::string nextTitle = "Практика по вашим навыкам";
    std::string nextExpression;
    if (!taskIds.empty() && position < static_cast<int>(taskIds.size())) {
        TaskVersion next = context.school.task(taskIds[position]);
        nextTitle = next.title;
        if (!next.parts.empty())
            nextExpression = next.parts.front().displayExpression.empty()
                                 ? next.parts.front().expression
                                 : next.parts.front().displayExpression;
    }
    note("АЛГЕБРА / ВАШЕ ЗАНЯТИЕ", 0, 0, 650);
    heading(ongoing ? "Продолжим практику." : "Немного математики. В своём темпе.", 0, 35, 36);
    note(ongoing ? "Занятие сохранено. Можно продолжить с того же места."
                 : "Выберите тему: потренируйтесь с разбором ошибок или проверьте знания.",
         0, 94, 650);
    surface(0, 144, 650, 236, Theme::pale);
    note(ongoing ? "СЛЕДУЮЩИЙ ШАГ / " + std::to_string(std::min(position + 1, total)) + " ИЗ " +
                       std::to_string(total)
                 : "ТРЕНИРОВКА ИЛИ ПРОВЕРКА ЗНАНИЙ",
         28, 169, 590);
    text(ongoing ? nextTitle : "Что хотите изучить сегодня?", 28, 206, 24, 590, 2);
    if (nextExpression.empty())
        note(ongoing ? "Задание будет выбрано по истории решений."
                     : "Одна тема, несколько тем или отдельный навык.",
             28, 274, 590);
    else {
        FormulaView *formula = add(new FormulaView(headingFace(), 590, 56), 28, 255);
        formula->setFeatured(true);
        formula->setFormula(nextExpression);
    }
    button(
        ongoing ? "Продолжить занятие →" : "Выбрать темы →", 28, 320, 260,
        [this] { startShortLesson(); }, true);
    button("Другие темы и режим", 310, 320, 240, [this] { settings(); });
    text(ongoing ? "План занятия" : "Как проходит тренировка", 0, 418, 20);
    if (taskIds.empty()) {
        note("Вы выбираете темы. После ошибки можно разобрать необходимые навыки на коротких "
             "упражнениях, затем вернуться к исходной задаче.",
             0, 470, 620);
    } else {
        int first = std::max(0, position - 1);
        for (int i = first;
             i < std::min(static_cast<int>(taskIds.size()), std::min(total, first + 3)); ++i) {
            float y = 455 + (i - first) * 50.f;
            surface(0, y, 650, 1, Theme::line);
            note(std::to_string(i + 1), 0, y + 16, 35);
            text(context.school.task(taskIds[i]).title, 40, y + 13, 16, 470, 1);
            note(i < position    ? "Завершено"
                 : i == position ? "Сейчас"
                                 : "Впереди",
                 535, y + 16, 115);
        }
    }
    surface(688, 0, 408, 2, Theme::ink);
    text(context.profileName +
             (context.profile == "demo-student" ? " / учебные данные" : " / ученик"),
         688, 25, 14, 408, 1);
    note(ongoing ? "ХОД ЗАНЯТИЯ" : "ПОСЛЕДНЕЕ ЗАНЯТИЕ", 688, 76, 408);
    text(progress.lesson.id.empty() ? "Пока нет результатов"
                                    : std::to_string(progress.finishedTasks) + " из " +
                                          std::to_string(progress.totalTasks) + " завершено",
         688, 108, 27, 408, 1);
    progressBar(progress.totalTasks == 0 ? 0.f
                                         : float(progress.finishedTasks) / progress.totalTasks,
                688, 155, 408);
    text(progress.lesson.id.empty() ? "Баллы появятся после решения"
                                    : "Баллы: " + lessonScore(progress),
         688, 183, 20, 408, 1);
    note(lessonNote(progress), 688, 222, 408);
    if (progress.lesson.settings.mode == SessionMode::Dynamic && !progress.lesson.id.empty())
        note("Максимум баллов указан за выданные задания.", 688, 283, 408);
    surface(688, 325, 408, 1, Theme::line);
    const auto snapshot = context.school.snapshot(context.profile);
    auto skills = context.school.skills();
    std::string skillId = "fractions", skillName = "Дроби";
    for (const auto &entry : context.school.history(context.profile)) {
        if (!entry.result.finished)
            continue;
        TaskVersion task = context.school.task(entry.taskId);
        if (!task.parts.empty() && !task.parts.front().skills.empty()) {
            skillId = task.parts.front().skills.front();
            break;
        }
    }
    for (const auto &skill : skills)
        if (skill.id == skillId)
            skillName = skill.name;
    SkillState skill;
    auto found = snapshot.skills.find(skillId);
    if (found != snapshot.skills.end())
        skill = found->second;
    note("НАВЫК / " + skillName, 688, 350, 408);
    text(progress.hidden ? "Проверка отложена" : skillEstimate(skill), 688, 390, 34, 408, 1);
    note(progress.hidden ? "Оценка навыка появится вместе с результатом занятия."
                         : skillEvidence(skill),
         688, 447, 408);
    note("Баллы за ответ и освоение темы — разные показатели.", 688, 514, 408);
    button("Подробнее о прогрессе →", 688, 573, 330, [this] { context.page = "progress"; });
}
void HomeScreen::teacherHome() {
    clear();
    title = "";
    note("КАБИНЕТ УЧИТЕЛЯ", 0, 0, 650);
    heading("Подготовьте следующее занятие.", 0, 35, 36);
    note("Готовые задания, свои примеры и результаты учеников.", 0, 94, 650);
    text("Банк заданий", 0, 159, 23);
    int published = 0, drafts = 0;
    for (const auto &task : context.school.tasks()) {
        if (task.archived)
            continue;
        if (task.published)
            ++published;
        else
            ++drafts;
    }
    surface(0, 203, 620, 1, Theme::line);
    text("Опубликовано: " + std::to_string(published), 0, 230, 20);
    text("Черновиков: " + std::to_string(drafts), 0, 270, 20);
    button("Открыть банк заданий →", 0, 324, 300, [this] { context.page = "bank"; }, true);
    button("Импортировать пакет", 320, 324, 300, [this] { context.page = "packages"; });
    surface(0, 400, 620, 1, Theme::line);
    text("Занятие для ученика", 0, 429, 23);
    note("Выберите профиль справа, затем составьте набор заданий. Ученик откроет его в своём "
         "кабинете.",
         0, 473, 620);
    button("Подготовить занятие →", 0, 548, 300, [this] {
        if (context.profile.empty())
            throw std::runtime_error("Сначала выберите ученика справа");
        settings();
    });
    button("Ручная проверка →", 320, 548, 300, [this] { reviewQueue(); });
    surface(688, 0, 408, 2, Theme::ink);
    note("УЧЕНИКИ И РЕЗУЛЬТАТЫ", 688, 25, 408);
    auto profiles = context.school.profiles();
    std::vector<std::string> names;
    int selected = -1;
    for (std::size_t i = 0; i < profiles.size(); ++i) {
        names.push_back(profiles[i].second);
        if (profiles[i].first == context.profile)
            selected = static_cast<int>(i);
    }
    ListBox *students = list(names, 688, 63, 408, 200);
    students->select(selected);
    students->onChange = [this, profiles](int index) {
        defer([this, profiles, index] {
            context.selectProfile(profiles[index].first, profiles[index].second);
            teacherHome();
        });
    };
    text(context.profile.empty() ? "Выберите ученика" : context.profileName, 688, 302, 22, 408, 2);
    note(context.profile == "demo-student"
             ? "Демонстрационные данные для знакомства с приложением."
             : "Здесь доступны профили, созданные на этом компьютере.",
         688, 365, 408);
    button("История решений →", 688, 439, 300, [this] { context.page = "results"; });
    button("Навыки ученика →", 688, 493, 300, [this] { context.page = "progress"; });
    button("Управление профилями", 688, 547, 300, [this] { manageProfiles(); });
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
            responses.push_back(response.partId + ": " +
                                (response.answer.text.empty() ? "График" : response.answer.text) +
                                " · " + response.result.message);
        list(responses, 590, 452, 350, 107);
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
