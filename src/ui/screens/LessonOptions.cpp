#include "LessonSetupScreen.h"
#include "LessonScreen.h"
#include "io/Files.h"
namespace exam {
namespace ui {
void LessonSetupScreen::options() {
    clear();
    title = "Условия занятия";
    text("Ограничение времени, минут (0 — без ограничения)", 0, 0, 20);
    TextField *minutes =
        field(std::to_string(context.settings.timeLimitSeconds / 60), 0, 45, 180, 4);
    auto readTime = [this, minutes] {
        double value = readNumber(minutes->getText());
        if (value < 0 || value > 1440 || value != static_cast<int>(value))
            throw std::runtime_error("Время: целое число от 0 до 1440 минут");
        context.settings.timeLimitSeconds = static_cast<int>(value) * 60;
    };
    text("Что происходит при выходе", 0, 115, 20);
    ListBox *policy = list({"Можно поставить на паузу", "Время идёт и после выхода",
                            "Нельзя продолжить после закрытия"},
                           0, 155, 540, 120);
    policy->select(static_cast<int>(context.settings.resumePolicy));
    policy->onChange = [this](int value) {
        context.settings.resumePolicy = static_cast<ResumePolicy>(value);
    };
    auto hints = add(new Toggle(font, "Разрешить подсказки", 540), 0, 315);
    hints->setValue(context.settings.helpAllowed);
    hints->onChange = [this](bool value) { context.settings.helpAllowed = value; };
    if (source == 2)
        button(context.settings.training ? "Разбор ошибок: включён" : "Разбор ошибок: выключен", 0,
               375, 540, [this, readTime] {
                   readTime();
                   context.settings.training = !context.settings.training;
                   context.settings.diagnosisAllowed = context.settings.training;
                   if (context.settings.training)
                       context.settings.feedback = FeedbackPolicy::AfterStep;
                   options();
               });
    if (source == 2)
        note("Разбор добавляет упражнения на нужные навыки.", 0, 440, 540);
    button("Сохранить как шаблон", 600, 0, 340, [this, readTime] {
        readTime();
        saveTemplateView();
    });
    button("Открыть шаблон", 600, 62, 340, [this, readTime] {
        readTime();
        templates();
    });
    button(
        "Применить и вернуться", 0, 550, 320,
        [this, readTime] {
            readTime();
            review();
        },
        true);
}
void LessonSetupScreen::saveTemplateView() {
    clear();
    title = "Сохранить шаблон";
    text("Название", 0, 20, 18);
    TextField *name = field("", 0, 60, 540, 100);
    auto save = button("Сохранить шаблон", 0, 140, 540, [this, name] {
        LessonTemplate value;
        value.id = newId("template");
        value.name = name->getText();
        value.settings = context.settings;
        if (value.settings.training && value.settings.targetSkills.empty())
            for (const auto &skill : context.school.skills())
                value.settings.targetSkills.push_back(skill.id);
        if (source != 2)
            for (auto &task : selectedTasks) {
                if (!task.published) {
                    context.school.saveDraft(task);
                    context.school.publish(task.id);
                    task.published = true;
                }
                value.tasks.push_back(task.id);
            }
        context.school.saveTemplate(value);
        status = "Шаблон сохранён. Занятие ещё не назначено.";
    });
    name->onEnter = save->onClick;

    note("Шаблон сохранит состав и условия занятия. Его можно выдать другому ученику.", 0, 235,
         800);
    button("← К условиям", 0, 550, 300, [this] { options(); });
}
void LessonSetupScreen::templates() {
    clear();
    title = "Сохранённые занятия";
    auto values = context.school.lessonTemplates();
    std::vector<std::string> rows;
    for (const auto &value : values)
        rows.push_back(value.name + (value.settings.mode == SessionMode::Fixed
                                         ? " · фиксированный вариант"
                                         : " · автоподбор"));
    ListBox *choices = list(rows, 0, 40, 940, 400);
    choices->setRowHeight(56);
    choices->setEmptyText(
        "Сохранённых занятий пока нет. Создайте состав и сохраните его в условиях занятия.");
    button(
        "Выбрать и проверить", 0, 550, 350,
        [this, choices, values] {
            if (choices->selection() < 0)
                throw std::runtime_error("Выберите шаблон");
            const auto &value = values[choices->selection()];
            context.settings = value.settings;
            skill =
                value.settings.targetSkills.size() == 1 ? value.settings.targetSkills.front() : "";
            source = value.settings.mode == SessionMode::Fixed ? 0 : 2;
            amount = std::to_string(value.settings.taskLimit);
            selectedTasks.clear();
            for (const auto &id : value.tasks)
                selectedTasks.push_back(context.school.task(id));
            review();
        },
        true);
    button("← Назад", 600, 550, 300, [this] { options(); });
}
void LessonScreen::tick() {
    if (timer.getElapsedTime().asSeconds() < 1 || !context.current.available)
        return;
    timer.restart();
    SessionTime time = context.school.lessonTime();
    if (!time.allowed && !time.paused) {
        context.current = context.school.next();
        refresh();
        status = time.message;
    } else if (time.remainingSeconds >= 0) {
        clockText = "Осталось: " + std::to_string(time.remainingSeconds / 60) + " мин " +
                    std::to_string(time.remainingSeconds % 60) + " сек";
        if (time.remainingSeconds <= 60)
            clockText += " — завершайте ответ";
    }
}
} // namespace ui
} // namespace exam
