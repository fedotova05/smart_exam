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
    minutes->onChange = [this, minutes] {
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
    button(context.settings.helpAllowed ? "Подсказки: разрешены" : "Подсказки: отключены", 0, 315,
           540, [this] {
               context.settings.helpAllowed = !context.settings.helpAllowed;
               options();
           });
    if (source == 2)
        button(context.settings.training ? "Разбор ошибок: включён" : "Разбор ошибок: выключен", 0,
               375, 540, [this] {
                   context.settings.training = !context.settings.training;
                   context.settings.diagnosisAllowed = context.settings.training;
                   if (context.settings.training)
                       context.settings.feedback = FeedbackPolicy::AfterStep;
                   options();
               });
    note("Разбор добавляет упражнения на необходимые навыки. Он работает с автоподбором и "
         "немедленной проверкой. Подсказки настраиваются отдельно.",
         0, 440, 540);
    text("Шаблоны", 600, 0, 24);
    note("Фиксированный вариант сохраняет точные задания. Шаблон автоподбора сохраняет число "
         "заданий и условия, а состав зависит от ученика.",
         600, 50, 340);
    text("Название", 600, 190, 18);
    TextField *name = field("", 600, 230, 340, 100);
    button("Сохранить шаблон", 600, 295, 340, [this, name] {
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
    button("Открыть сохранённые", 600, 355, 340, [this] { templates(); });
    note("После назначения условия занятия зафиксированы. Изменение шаблона не меняет выданное "
         "занятие.",
         600, 425, 340);
    button("← К проверке занятия", 0, 550, 320, [this] { review(); });
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
        status = "Осталось: " + std::to_string(time.remainingSeconds / 60) + " мин " +
                 std::to_string(time.remainingSeconds % 60) + " сек";
        if (time.remainingSeconds <= 60)
            status += " — завершайте ответ";
    }
}
} // namespace ui
} // namespace exam
