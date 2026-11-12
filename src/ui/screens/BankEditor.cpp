#include "BankScreen.h"
#include "ui/FormulaView.h"
#include "ui/widgets/TextPanel.h"
#include "math/checkers/EquationChecker.h"
#include "io/Files.h"
#include <algorithm>
#include <cmath>
namespace exam {
namespace ui {
std::string BankScreen::typeLabel(PartKind kind) {
    const char *names[] = {"Вычислить точно",        "Решить уравнение",        "Построить прямую",
                           "Построить параболу",     "Вычислить с округлением", "Выбрать варианты",
                           "Упростить выражение с x"};
    return names[static_cast<int>(kind)];
}
TaskPart BankScreen::examplePart(PartKind kind) {
    TaskPart part;
    part.kind = kind;
    const char *prompts[] = {"Вычислите значение. Ответ запишите дробью.",
                             "Решите уравнение. Укажите все корни.",
                             "Постройте график функции.",
                             "Постройте график функции.",
                             "Вычислите значение с указанной точностью.",
                             "Выберите все верные утверждения.",
                             "Раскройте скобки и приведите подобные слагаемые."};
    const char *expressions[] = {"1/2+1/3", "x^2-4=0", "2*x+1", "x^2-4", "sqrt(2)", "", "(x+1)^2"};
    part.prompt = prompts[static_cast<int>(kind)];
    part.expression = expressions[static_cast<int>(kind)];
    if (kind == PartKind::Number)
        part.decimalPlaces = 2;
    if (kind == PartKind::Choice)
        part.choices.resize(2);
    return part;
}
void BankScreen::readScores() {
    for (auto &part : draft.parts) {
        auto found = scoreDrafts.find(part.id);
        if (found == scoreDrafts.end())
            continue;
        double value = readNumber(found->second);
        if (!std::isfinite(value) || value < 1 || value > 100 || value != int(value))
            throw std::runtime_error("Баллы за часть " + part.id + ": целое число от 1 до 100");
        part.maxScore = int(value);
    }
}
void BankScreen::chooseType(int selected) {
    if (selected < 0)
        selected = static_cast<int>(draft.parts[partIndex].kind);
    clear();
    title = "Тип задания";
    note("Выберите, что должен сделать ученик.", 0, 0, 940);
    std::vector<std::string> names;
    for (int i = 0; i < 7; ++i)
        names.push_back(typeLabel(static_cast<PartKind>(i)));
    auto types = list(names, 0, 55, 430, 420);
    types->setRowHeight(60);
    types->select(selected);
    types->onChange = [this](int i) { defer([this, i] { chooseType(i); }); };
    const char *details[] = {
        "Ученик вычисляет значение и вводит точный ответ: число, дробь или корень. Проверка "
        "принимает равные выражения.",
        "Ученик находит корни уравнения. Ответ вычисляется по уравнению; вводить правильные корни "
        "учителю не нужно.",
        "Ученик отмечает две точки на координатной сетке. Укажите линейную функцию без «y =».",
        "Ученик строит параболу по коэффициентам. Укажите квадратную функцию без «y =».",
        "Ученик вычисляет значение. Вы задаёте число знаков после запятой и при необходимости "
        "допустимую погрешность.",
        "Ученик отмечает один или несколько вариантов. Вы вводите тексты вариантов и отмечаете "
        "правильные.",
        "Ученик преобразует выражение с переменной x. Равносильная запись считается правильной."};
    text(names[selected], 480, 55, 25, 460, 2);
    add(new TextPanel(font, details[selected], 460, 160), 480, 130);
    auto sample = examplePart(static_cast<PartKind>(selected));
    if (!sample.expression.empty()) {
        note("ПРИМЕР", 480, 320, 460);
        auto formula = add(new FormulaView(headingFace(), 460, 100), 480, 355);
        formula->setFormula(sample.expression);
    }
    button(
        "Выбрать тип", 0, 550, 280, [this, selected] { changeType(selected); }, true);
    button("Отмена", 310, 550, 220, [this] { editor(); });
    focusWidget(types);
}
void BankScreen::changeType(int kind) {
    auto &current = draft.parts[partIndex];
    if (int(current.kind) == kind) {
        editor();
        return;
    }
    const std::string id = current.id;
    typeDrafts[id][int(current.kind)] = current;
    auto found = typeDrafts[id].find(kind);
    if (found != typeDrafts[id].end())
        current = found->second;
    else {
        auto next = examplePart(static_cast<PartKind>(kind));
        next.id = id;
        next.skills = current.skills;
        next.maxScore = current.maxScore;
        current = next;
    }
    editor();
    status = "Тип изменён. При переключении обратно прежний текст этой части восстановится.";
}
void BankScreen::editor() {
    clear();
    title = "Редактор задания";
    if (draft.parts.empty())
        return;
    auto &part = draft.parts[partIndex];
    text("Название в банке", 0, 0, 16);
    auto name = field(draft.title, 0, 26, 620, 160);
    name->onChange = [this, name] { draft.title = name->getText(); };
    button(typeLabel(part.kind) + " · изменить тип", 0, 84, 620, [this] { chooseType(); });
    text(draft.parts.size() > 1 ? "Вопрос этой части" : "Вопрос для ученика", 0, 148, 18);
    auto prompt = field(part.prompt, 0, 180, 620, 2048);
    prompt->onChange = [this, prompt] { draft.parts[partIndex].prompt = prompt->getText(); };
    if (part.kind == PartKind::Choice) {
        text("Варианты ответа", 0, 248, 18);
        button(
            "Редактировать варианты и верные ответы", 0, 282, 620, [this] { answerSettings(); },
            true);
        std::vector<std::string> rows;
        for (std::size_t i = 0; i < part.choices.size(); ++i)
            rows.push_back(std::to_string(i + 1) + ". " +
                           (part.choices[i].empty() ? "Не заполнен" : part.choices[i]));
        auto options = list(rows, 0, 348, 620, 168);
        options->setRowHeight(56);
        note("Правильных вариантов отмечено: " + std::to_string(part.correctChoices.size()), 0, 525,
             620);
    } else {
        const bool graph = part.kind == PartKind::Line || part.kind == PartKind::Parabola;
        text(part.kind == PartKind::Equation ? "Уравнение"
             : graph                         ? "Функция y ="
                                             : "Выражение для ученика",
             0, 248, 18);
        auto expression = field(part.expression, 0, 282, 620, 512);
        expression->setPlaceholder(examplePart(part.kind).expression);
        auto formula = add(new FormulaView(headingFace(), 620, 118), 0, 382);
        formula->setFeatured(true);
        formula->setFormula(part.expression);
        expression->onChange = [this, expression, formula] {
            draft.parts[partIndex].expression = expression->getText();
            if (!isLinearEquation(expression->getText()))
                draft.parts[partIndex].requireSteps = false;
            formula->setFormula(expression->getText());
        };
        note(graph ? (part.kind == PartKind::Parabola
                          ? "Только правая часть: x^2-4. Умножение — *, степень — ^."
                          : "Только правая часть: 2*x+1. Умножение — *, степень — ^.")
             : part.kind == PartKind::Equation
                 ? "Например: 2*x+3=11. Правильный ответ вычисляется автоматически."
                 : "Дробь: 1/2. Корень: sqrt(2). Степень: x^2.",
             0, 336, 620);
    }
    surface(645, 0, 1, 530, Theme::line);
    button("← В банк", 670, 0, 270, [this] {
        ask("Вернуться в банк? Несохранённые изменения будут потеряны.", [this] {
            editing = false;
            refresh();
        });
    });
    text("Проверяемые навыки", 670, 70, 16);
    button("Выбрано: " + std::to_string(part.skills.size()) + " · изменить", 670, 98, 270,
           [this] { selectSkills(); });
    text("Баллы за ответ", 670, 162, 16);
    if (!scoreDrafts.count(part.id))
        scoreDrafts[part.id] = std::to_string(part.maxScore);
    auto score = field(scoreDrafts[part.id], 670, 190, 100, 3);
    score->onChange = [this, score] { scoreDrafts[draft.parts[partIndex].id] = score->getText(); };
    note("От 1 до 100", 790, 204, 150);
    if (part.kind == PartKind::Number || part.kind == PartKind::Expression)
        button("Точность ответа", 670, 256, 270, [this] { answerSettings(); });
    else if (part.kind == PartKind::Line || part.kind == PartKind::Parabola)
        button("Точность графика", 670, 256, 270, [this] { answerSettings(); });
    else if (part.kind == PartKind::Equation)
        button("Проверка решения", 670, 256, 270, [this] { answerSettings(); });
    button("Части задания: " + std::to_string(draft.parts.size()), 670, 320, 270,
           [this] { partsEditor(); });
    button("Дополнительно", 670, 384, 270, [this] { extraSettings(); });
    note("Сначала сохраните черновик. Публикация делает задание доступным для занятий.", 670, 450,
         270);
    button("Сохранить черновик", 0, 566, 260, [this] {
        readScores();
        context.school.saveDraft(draft);
        status = "Черновик сохранён";
    });
    button("Предпросмотр", 280, 566, 240, [this] {
        readScores();
        preview();
    });
    button(
        "Опубликовать", 670, 566, 270,
        [this] {
            readScores();
            auto errors = context.school.validate(draft);
            if (!errors.empty())
                throw std::runtime_error(errors.front());
            context.school.saveDraft(draft);
            context.school.publish(draft.id);
            editing = false;
            refresh();
            status = "Задание опубликовано";
        },
        true);
}
void BankScreen::partsEditor() {
    clear();
    title = "Части задания";
    note("Одна часть — один вопрос и ответ. Дополнительные части нужны только для составного "
         "задания.",
         0, 0, 940);
    std::vector<std::string> rows;
    for (std::size_t i = 0; i < draft.parts.size(); ++i)
        rows.push_back(std::to_string(i + 1) + ". " + typeLabel(draft.parts[i].kind));
    auto parts = list(rows, 0, 60, 430, 360);
    parts->setRowHeight(60);
    parts->select(partIndex);
    button(
        "Открыть часть", 0, 450, 210,
        [this, parts] {
            partIndex = parts->selection();
            editor();
        },
        true);
    button("Добавить часть", 230, 450, 200, [this] {
        auto part = examplePart(PartKind::Equation);
        part.id = newId("part");
        part.skills = draft.parts[partIndex].skills;
        draft.parts.push_back(part);
        partIndex = int(draft.parts.size()) - 1;
        editor();
    })->setEnabled(draft.parts.size() < 100);
    button("Удалить выбранную", 0, 510, 430, [this, parts] {
        const int index = parts->selection();
        ask("Удалить выбранную часть вместе с её вопросом и настройками?", [this, index] {
            scoreDrafts.erase(draft.parts[index].id);
            typeDrafts.erase(draft.parts[index].id);
            draft.parts.erase(draft.parts.begin() + index);
            partIndex = 0;
            partsEditor();
        });
    })->setEnabled(draft.parts.size() > 1);
    if (draft.parts.size() > 1 || !draft.description.empty()) {
        text("Общее условие для всех частей", 480, 60, 18);
        auto description = field(draft.description, 480, 100, 460, 2048);
        description->onChange = [this, description] { draft.description = description->getText(); };
        note("Показывается перед вопросом каждой части. Можно оставить пустым.", 480, 160, 460);
    }
    if (draft.parts.size() > 1 || !draft.commonFunction.empty()) {
        text("Общая функция (необязательно)", 480, 250, 18);
        auto function = field(draft.commonFunction, 480, 290, 460, 512);
        function->onChange = [this, function] { draft.commonFunction = function->getText(); };
        note("Для связанных вопросов о корнях и графике одной функции. Например: x^2-4. При "
             "публикации проверяется совпадение.",
             480, 350, 460);
    }
    button("← К заданию", 660, 566, 280, [this] { editor(); });
}
void BankScreen::extraSettings() {
    clear();
    title = "Дополнительные параметры";
    const auto &part = draft.parts[partIndex];
    text("Формула условия (необязательно)", 0, 0, 19);
    auto display = field(part.displayExpression, 0, 40, 940, 512);
    note(part.kind == PartKind::Choice
             ? "Показывается над вариантами. Для текстового вопроса оставьте пустым."
             : "Меняет только вид условия. Проверка использует выражение из основной формы.",
         0, 100, 940);
    text("Изображения из папки data", 0, 180, 19);
    auto resources = field(joinCodes(draft.resources), 0, 220, 940, 1024);
    note("Относительные пути через запятую. Например: images/task.png. Изображения доступны "
         "ученику по кнопке «Рисунок».",
         0, 280, 940);
    text("Рейтинг сложности", 0, 365, 19);
    auto difficulty = field(std::to_string(draft.difficulty), 0, 405, 220, 24);
    note("От 0 до 3000, по умолчанию 1000. Чем выше, тем сложнее задание при подборе. Можно "
         "оставить исходное значение.",
         260, 405, 680);
    button(
        "Применить", 0, 566, 260,
        [this, display, resources, difficulty] {
            double rating = readNumber(difficulty->getText());
            if (!std::isfinite(rating) || rating < 0 || rating > 3000)
                throw std::runtime_error("Рейтинг сложности: число от 0 до 3000");
            auto paths = splitCodes(resources->getText());
            for (const auto &path : paths)
                if (!safeResourcePath(path))
                    throw std::runtime_error("Укажите путь к изображению внутри папки data");
            draft.parts[partIndex].displayExpression = display->getText();
            draft.resources = paths;
            draft.difficulty = rating;
            editor();
        },
        true);
    button("Отмена", 280, 566, 220, [this] { editor(); });
}
void BankScreen::preview() {
    clear();
    title = "Предпросмотр задания";
    const auto &part = draft.parts[partIndex];
    text(draft.title, 0, 0, 24, 940, 2);
    add(new TextPanel(font,
                      draft.description + (draft.description.empty() ? "" : "\n") + part.prompt,
                      620, 110),
        0, 72);
    auto expression = part.displayExpression.empty() ? part.expression : part.displayExpression;
    if (!expression.empty()) {
        auto formula = add(new FormulaView(headingFace(), 620, 110), 0, 200);
        formula->setFeatured(true);
        formula->setFormula(expression);
    }
    if (part.kind == PartKind::Choice) {
        auto variants = list(part.choices, 0, expression.empty() ? 200 : 330, 620,
                             expression.empty() ? 288 : 168);
        variants->setRowHeight(56);
        variants->setCheckable(true);
        for (int correct : part.correctChoices)
            variants->setChecked(correct - 1, true);
    }
    text(typeLabel(part.kind), 670, 72, 21, 270, 3);
    note("Баллов: " + std::to_string(part.maxScore) +
             "\nНавыков: " + std::to_string(part.skills.size()),
         670, 175, 270);
    if (part.kind == PartKind::Choice)
        note("Здесь отмечены правильные варианты. Ученику они заранее не показываются.", 670, 260,
             270);
    auto errors = context.school.validate(draft);
    add(new TextPanel(font, errors.empty() ? "Задание готово к публикации" : errors.front(), 940,
                      60),
        0, 498);
    button("← К редактированию", 0, 576, 300, [this] { editor(); });
}
} // namespace ui
} // namespace exam
