#include "LessonSetupScreen.h"
#include "ui/FormulaView.h"
#include "generation/Generator.h"
#include "io/Files.h"
#include <algorithm>
#include <ctime>
namespace exam {
namespace ui {
namespace {
sf::String lowercase(const std::string &text) {
    sf::String value = unicode(text);
    for (std::size_t i = 0; i < value.getSize(); ++i) {
        if ((value[i] >= 'A' && value[i] <= 'Z') || (value[i] >= 0x410 && value[i] <= 0x42f))
            value[i] += 32;
        if (value[i] == 0x401)
            value[i] = 0x451;
    }
    return value;
}
int maximum(const TaskVersion &task) {
    int sum = 0;
    for (const auto &part : task.parts)
        sum += part.maxScore;
    return sum;
}
} // namespace
LessonSetupScreen::LessonSetupScreen(const sf::Font &font, AppContext &state)
    : Screen(font), context(state), amount("3"), source(0), family(0), basketIndex(0),
      seed(static_cast<unsigned>(std::time(0))) {}
int LessonSetupScreen::taskCount() const {
    double count = readNumber(amount);
    if (count < 1 || count > 20 || count != static_cast<int>(count))
        throw std::runtime_error("Введите целое число заданий от 1 до 20");
    return static_cast<int>(count);
}
void LessonSetupScreen::refresh() {
    if (profile != context.profile) {
        profile = context.profile;
        selectedTasks.clear();
        skill.clear();
        query.clear();
        selectedTask.clear();
        basketIndex = 0;
        source = 0;
        amount = "3";
    }
    clear();
    title = "Собрать занятие";
    if (context.profile.empty()) {
        text("Сначала выберите ученика в кабинете.", 0, 20);
        button("В кабинет", 0, 100, 220, [this] { context.page = "home"; });
        return;
    }
    text("Для кого: " + context.profileName, 0, 0, 18, 940, 1);
    if (context.role == UserRole::Teacher)
        button("Сохранённые занятия", 620, 73, 320, [this] { templates(); });
    note("1. ВЫБЕРИТЕ ЗАДАНИЯ     →     2. ПРОВЕРЬТЕ И ЗАПУСТИТЕ", 0, 38);
    const char *names[] = {"По темам", "Создать примеры",
                           context.role == UserRole::Teacher ? "По навыкам ученика"
                                                             : "По моим навыкам"};
    for (int i = 0; i < 3; ++i)
        button(
            names[i], i * 195.f, 73, 185,
            [this, i] {
                source = i;
                refresh();
            },
            source == i);
    surface(599, 131, 1, 426, Theme::line);
    if (source == 0)
        bank();
    else if (source == 1)
        generator();
    else
        adaptive();
    basket();
    button("← В кабинет", 0, 580, 220, [this] { context.page = "home"; });
    button("Далее: проверить занятие →", 620, 580, 320, [this] { review(); }, true);
}
void LessonSetupScreen::bank() {
    text("Тема практики", 0, 137, 18);
    auto skills = context.school.skills();
    std::vector<std::string> names = {"Все темы"};
    int chosen = 0;
    for (std::size_t i = 0; i < skills.size(); ++i) {
        names.push_back(skills[i].name);
        if (skills[i].id == skill)
            chosen = static_cast<int>(i + 1);
    }
    ListBox *topics = list(names, 0, 170, 230, 240);
    topics->select(chosen);
    topics->onChange = [this, skills](int index) {
        defer([this, skills, index] {
            skill = index == 0 ? "" : skills[index - 1].id;
            selectedTask.clear();
            refresh();
        });
    };
    TextField *search = field(query, 250, 170, 330, 100);
    search->setPlaceholder("Поиск по названию или условию");
    search->onChange = [this, search] { query = search->getText(); };
    search->onEnter = [this] { defer([this] { refresh(); }); };
    button("Найти", 460, 218, 120, [this] { refresh(); });
    std::vector<TaskVersion> tasks;
    std::vector<std::string> rows;
    int selected = 0;
    for (const auto &task : context.school.tasks()) {
        if (!task.published || task.archived)
            continue;
        bool matchesSkill = skill.empty();
        std::string contents = task.title + " " + task.description;
        for (const auto &part : task.parts) {
            contents += " " + part.prompt;
            if (std::find(part.skills.begin(), part.skills.end(), skill) != part.skills.end())
                matchesSkill = true;
        }
        if (!matchesSkill || lowercase(contents).find(lowercase(query)) == sf::String::InvalidPos)
            continue;
        if (task.id == selectedTask)
            selected = static_cast<int>(tasks.size());
        tasks.push_back(task);
        rows.push_back(task.title);
    }
    ListBox *choices = list(rows, 250, 270, 330, 160);
    choices->select(tasks.empty() ? -1 : selected);
    choices->onChange = [this, tasks](int index) {
        defer([this, tasks, index] {
            selectedTask = tasks[index].id;
            refresh();
        });
    };
    note("Готовые задания из банка. Выберите тему, затем задание справа.", 0, 434, 230);
    if (!tasks.empty()) {
        const auto &task = tasks[selected];
        note("Частей: " + std::to_string(task.parts.size()) +
                 ". Баллов: " + std::to_string(maximum(task)),
             250, 445, 330);
        FormulaView *formula = add(new FormulaView(headingFace(), 330, 60), 250, 471);
        formula->setFormula(task.parts.front().expression);
    }
    button("Добавить выбранное →", 250, 535, 330, [this, choices, tasks] {
        if (choices->selection() < 0)
            throw std::runtime_error("Поиск ничего не нашёл. Выберите другую тему или запрос.");
        const TaskVersion &task = tasks[choices->selection()];
        for (const auto &old : selectedTasks)
            if (old.id == task.id)
                throw std::runtime_error("Это задание уже есть в занятии справа");
        if (selectedTasks.size() >= 100)
            throw std::runtime_error("В одном занятии не более 100 заданий");
        selectedTasks.push_back(task);
        basketIndex = static_cast<int>(selectedTasks.size()) - 1;
        refresh();
    });
}
void LessonSetupScreen::generator() {
    text("Что создать?", 0, 137, 18);
    const std::vector<std::string> names = {"Действия с дробями", "Линейные уравнения",
                                            "Квадратные уравнения", "Корни и прямая",
                                            "Корни и парабола"};
    const char *descriptions[] = {
        "Сложение двух дробей с новыми числами. Ответ — точное значение. Одна часть, 1 балл.",
        "Новое линейное уравнение. Найдите x. Одна часть, 1 балл.",
        "Новое квадратное уравнение. Найдите все корни, в том числе с квадратным корнем. Одна "
        "часть, 1 балл.",
        "Одна функция, два вопроса: найти корень и построить прямую. Две части, 2 балла.",
        "Одна функция, два вопроса: найти корни и построить параболу. Две части, 2 балла."};
    ListBox *families = list(names, 0, 170, 270, 200);
    families->select(family);
    families->onChange = [this](int value) {
        defer([this, value] {
            family = value;
            refresh();
        });
    };
    text(descriptions[family], 295, 170, 17, 285, 7);
    text("Сколько новых заданий?", 295, 359, 17, 285, 2);
    TextField *count = field(amount, 295, 408, 100, 2);
    count->onChange = [this, count] { amount = count->getText(); };
    note("От 1 до 20", 413, 420, 167);
    button(
        "Создать и добавить в занятие →", 0, 475, 580,
        [this, names] {
            const int count = taskCount();
            if (selectedTasks.size() + count > 100)
                throw std::runtime_error("В одном занятии не более 100 заданий");
            std::set<std::string> expressions;
            for (const auto &task : selectedTasks)
                expressions.insert(task.parts.front().expression);
            std::vector<TaskVersion> generated;
            for (int i = 0; i < count; ++i) {
                GenerationRequest request;
                request.id = newId("practice");
                request.family = static_cast<Family>(family);
                request.seed = seed++;
                request.previousExpressions = expressions;
                TaskVersion task = Generator().generateTask(request);
                task.title =
                    names[family] + " · пример " + std::to_string(selectedTasks.size() + i + 1);
                expressions.insert(task.parts.front().expression);
                generated.push_back(task);
            }
            basketIndex = static_cast<int>(selectedTasks.size());
            selectedTasks.insert(selectedTasks.end(), generated.begin(), generated.end());
            refresh();
        },
        true);
    note("Генератор создаёт новые условия. До запуска занятия их можно убрать из списка справа.", 0,
         530, 580);
}
void LessonSetupScreen::adaptive() {
    heading("Программа выбирает задания", 0, 148, 25);
    text("Из опубликованного банка — по истории выбранного ученика. Сначала неизвестные "
         "или слабые навыки, затем закрепление. Учитываются сложность и повторения.",
         0, 202, 19, 570, 5);
    auto skills = context.school.skills();
    std::vector<std::string> names = {"Все навыки"};
    int selected = 0;
    for (std::size_t i = 0; i < skills.size(); ++i) {
        names.push_back(skills[i].name);
        if (skills[i].id == skill)
            selected = static_cast<int>(i + 1);
    }
    ListBox *scope = list(names, 0, 310, 570, 100);
    scope->select(selected);
    scope->onChange = [this, skills](int index) {
        skill = index == 0 ? "" : skills[index - 1].id;
        context.settings.targetSkills.clear();
    };
    text("Сколько заданий пройти?", 0, 420, 19);
    TextField *count = field(amount, 0, 460, 100, 2);
    count->onChange = [this, count] { amount = count->getText(); };
    note("От 1 до 20", 120, 473, 200);
}
void LessonSetupScreen::basket() {
    if (source == 2) {
        text("Автоматический подбор", 620, 137, 22, 320, 2);
        text("Состав занятия зависит от ответов ученика.\n\nВыбранные вручную задания сохраняются "
             "в черновике, но в этот режим не входят.",
             620, 210, 16, 320, 7);
        return;
    }
    text("В занятии: " + std::to_string(selectedTasks.size()), 620, 137, 22, 320, 1);
    int points = 0;
    std::vector<std::string> rows;
    for (std::size_t i = 0; i < selectedTasks.size(); ++i) {
        const auto &task = selectedTasks[i];
        points += maximum(task);
        rows.push_back(std::to_string(i + 1) + ". " + task.title);
    }
    if (basketIndex >= static_cast<int>(selectedTasks.size()))
        basketIndex = 0;
    ListBox *selected = list(rows, 620, 180, 320, 200);
    selected->select(rows.empty() ? -1 : basketIndex);
    selected->onChange = [this](int index) {
        defer([this, index] {
            basketIndex = index;
            refresh();
        });
    };
    note("Максимум: " + std::to_string(points) + " балл. Порядок — как в списке.", 620, 393, 320);
    if (!selectedTasks.empty()) {
        FormulaView *preview = add(new FormulaView(headingFace(), 320, 78), 620, 440);
        preview->setFormula(selectedTasks[basketIndex].parts.front().expression);
    }
    button("Убрать выбранное", 620, 530, 320, [this, selected] {
        if (selected->selection() >= 0)
            selectedTasks.erase(selectedTasks.begin() + selected->selection());
        refresh();
    });
}
void LessonSetupScreen::review() {
    if (source != 2 && selectedTasks.empty())
        throw std::runtime_error(
            "Добавьте задания: выберите их по теме или создайте новые примеры");
    context.settings.guided = false;
    if (source != 2) {
        context.settings.training = false;
        context.settings.targetSkills.clear();
    } else if (!skill.empty())
        context.settings.targetSkills = {skill};
    else if (context.settings.training && context.settings.targetSkills.empty())
        for (const auto &item : context.school.skills())
            context.settings.targetSkills.push_back(item.id);
    context.settings.mode = source == 2 ? SessionMode::Dynamic : SessionMode::Fixed;
    context.settings.taskLimit = source == 2 ? taskCount() : static_cast<int>(selectedTasks.size());
    clear();
    title = "Проверьте занятие";
    text("Ученик: " + context.profileName, 0, 0, 22, 940, 1);
    text("Заданий: " + std::to_string(context.settings.taskLimit), 0, 55, 22);
    note(source == 2 ? "Автоподбор из банка по истории ученика."
                     : "Все задания уже выбраны. Порядок показан слева.",
         0, 100, 550);
    std::vector<std::string> rows;
    int generated = 0;
    for (const auto &task : selectedTasks) {
        rows.push_back(task.title + " · " + std::to_string(maximum(task)) + " балл.");
        if (!task.published)
            ++generated;
    }
    if (source != 2)
        list(rows, 0, 150, 550, 280);
    else
        text("Первое задание будет выбрано при начале решения. Следующие — после завершения "
             "предыдущих.",
             0, 180, 21, 550, 5);
    text("Когда показывать проверку?", 600, 150, 20, 340, 2);
    ListBox *feedback =
        list({"Сразу после ответа", "После задания", "После всего занятия"}, 600, 213, 340, 120);
    feedback->select(static_cast<int>(context.settings.feedback));
    feedback->onChange = [this](int value) {
        context.settings.feedback = static_cast<FeedbackPolicy>(value);
    };
    note("Шаги необязательны и доступны только для линейных уравнений.", 600, 440, 340);
    std::string exitRule = context.settings.resumePolicy == ResumePolicy::ActiveTime
                               ? "Можно поставить на паузу."
                           : context.settings.resumePolicy == ResumePolicy::Deadline
                               ? "После выхода время продолжает идти."
                               : "После закрытия программы продолжение запрещено.";
    note((context.settings.timeLimitSeconds
              ? "Лимит: " + std::to_string(context.settings.timeLimitSeconds / 60) + " мин. "
              : "Без лимита времени. ") +
             exitRule,
         0, 470, 550);
    note("Сразу — удобно для тренировки.\nПосле занятия — для самостоятельной работы.", 600, 363,
         340);
    if (generated > 0 && source != 2)
        note("Новых примеров: " + std::to_string(generated) +
                 ". При запуске они сохранятся в банке.",
             0, 435, 550);
    note(context.role == UserRole::Teacher
             ? "После подготовки ученик войдёт в свой профиль и нажмёт «Продолжить занятие»."
             : "Результаты сохранятся в вашем профиле.",
         0, 515, 940);
    button("Условия и шаблоны", 270, 580, 310, [this] { options(); });
    button("← Изменить состав", 0, 580, 250, [this] { refresh(); });
    button(
        context.role == UserRole::Teacher ? "Подготовить для ученика →" : "Начать решение →", 600,
        580, 340, [this] { start(); }, true);
}
void LessonSetupScreen::start(bool replace) {
    SessionRecord previous = context.school.latestLesson(context.profile);
    if (!previous.id.empty() && !previous.closed)
        for (const auto &attempt : context.school.lessonAttempts(previous.id))
            if (!attempt.finished)
                throw std::runtime_error(
                    "Есть незавершённое занятие. Сначала продолжите его с главной.");
    if (!previous.id.empty() && !previous.closed && !replace) {
        ask("У ученика уже есть занятие. Закрыть его и заменить новым? Сохранённые ответы "
            "останутся в истории.",
            [this] { start(true); });
        return;
    }
    std::vector<std::string> ids;
    if (source != 2) {
        for (const auto &task : selectedTasks) {
            const auto errors = context.school.validate(task);
            if (!errors.empty())
                throw std::runtime_error(errors.front());
        }
        for (auto &task : selectedTasks) {
            if (!task.published) {
                context.school.saveDraft(task);
                context.school.publish(task.id);
                task.published = true;
            }
            ids.push_back(task.id);
        }
    }
    context.school.start(context.profile, context.settings, ids);
    selectedTasks.clear();
    context.drafts.clear();
    if (context.role == UserRole::Teacher) {
        clear();
        title = "Занятие подготовлено";
        heading("Теперь можно начинать.", 0, 40, 36);
        text("Ученик: " + context.profileName, 0, 112, 24, 900, 1);
        text("Ученик выбирает свой профиль при входе, затем нажимает «Продолжить занятие». "
             "Вы сможете увидеть результаты в кабинете.",
             0, 175, 21, 850, 4);
        button("В кабинет учителя →", 0, 320, 320, [this] { context.page = "home"; }, true);
    } else {
        context.current = context.school.next();
        context.page = "lesson";
    }
}
} // namespace ui
} // namespace exam
