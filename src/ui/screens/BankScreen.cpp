#include "BankScreen.h"
#include "ui/FormulaView.h"
#include "io/Files.h"
#include <algorithm>
#include <sstream>
namespace exam {
namespace ui {
void BankScreen::createTask(bool combined) {
    draft = TaskVersion();
    draft.id = newId("task");
    draft.title = combined ? "Корни и график функции" : "Новое задание";
    TaskPart part;
    part.id = "a";
    part.prompt = "Решите уравнение";
    part.expression = "x^2-4=0";
    auto skills = context.school.skills();
    if (!skills.empty())
        part.skills.push_back(skills.front().id);
    draft.parts.push_back(part);
    if (combined) {
        draft.commonFunction = "x^2-4";
        draft.description = "Найдите нули функции и постройте её график.";
        part.id = "b";
        part.kind = PartKind::Parabola;
        part.prompt = "Постройте параболу";
        part.expression = draft.commonFunction;
        draft.parts.push_back(part);
    }
    editing = true;
    partIndex = 0;
    editor();
}
void BankScreen::refresh() {
    if (context.role != UserRole::Teacher) {
        clear();
        title = "Банк заданий доступен учителю";
        return;
    }
    if (editing) {
        editor();
        return;
    }
    clear();
    title = "Банк заданий";
    TextField *search = field(query, 0, 0, 700, 120);
    search->setPlaceholder("Поиск по названию, условию или навыку");
    button("Найти", 720, 0, 220, [this, search] {
        query = search->getText();
        refresh();
    });
    search->onEnter = [this, search] {
        query = search->getText();
        defer([this] { refresh(); });
    };
    auto tasks = context.school.tasks();
    std::vector<TaskVersion> found;
    std::vector<std::string> rows;
    sf::String needle = unicode(query);
    for (std::size_t i = 0; i < needle.getSize(); ++i) {
        if (needle[i] >= 'A' && needle[i] <= 'Z')
            needle[i] += 32;
        if (needle[i] >= 0x410 && needle[i] <= 0x42f)
            needle[i] += 32;
    }
    for (const auto &task : tasks) {
        std::string contents = task.title + " " + task.description + " " + task.id;
        for (const auto &part : task.parts)
            contents += " " + part.prompt + " " + joinCodes(part.skills);
        sf::String haystack = unicode(contents);
        for (std::size_t i = 0; i < haystack.getSize(); ++i) {
            if (haystack[i] >= 'A' && haystack[i] <= 'Z')
                haystack[i] += 32;
            if (haystack[i] >= 0x410 && haystack[i] <= 0x42f)
                haystack[i] += 32;
        }
        if (haystack.find(needle) == sf::String::InvalidPos)
            continue;
        found.push_back(task);
        rows.push_back((task.archived    ? "Архив"
                        : task.published ? "Опубликовано"
                                         : "Черновик") +
                       std::string(" · ") + task.title +
                       " · частей: " + std::to_string(task.parts.size()));
    }
    ListBox *bank = list(rows, 0, 70, 940, 350);
    button("Открыть", 0, 445, 210, [this, bank, found] {
        if (bank->selection() < 0)
            return;
        draft = found[bank->selection()];
        if (draft.published) {
            draft = context.school.newVersion(draft.id);
        }
        editing = true;
        partIndex = 0;
        editor();
        status = "Опубликованное задание открывается как новая версия. Старая версия сохраняется.";
    });
    button("Новое задание", 230, 445, 220, [this] { createTask(false); }, true);
    button("Корни + график", 470, 445, 220, [this] { createTask(true); });
    button("В архив", 710, 445, 230, [this, bank, found] {
        if (bank->selection() < 0)
            return;
        std::string id = found[bank->selection()].id;
        ask("Убрать задание из подбора? История сохранится.", [this, id] {
            context.school.archive(id);
            refresh();
        });
    });
    text("Черновики можно исправлять. Для опубликованного задания создаётся новая версия. Проверка "
         "условий выполняется перед публикацией.",
         0, 520, 18, 900, 3);
}
void BankScreen::editor() {
    clear();
    title = "Редактор задания";
    text("Название", 0, 0, 16);
    TextField *name = field(draft.title, 0, 24, 620, 160);
    name->onChange = [this, name] { draft.title = name->getText(); };
    button("Назад в банк", 650, 24, 290, [this] {
        ask("Вернуться в банк? Несохранённые изменения будут потеряны.", [this] {
            editing = false;
            refresh();
        });
    });
    text("Общее условие", 0, 76, 16);
    TextField *description = field(draft.description, 0, 100, 940, 2048);
    description->onChange = [this, description] { draft.description = description->getText(); };
    text("Общая функция (необязательно)", 0, 152, 16);
    TextField *function = field(draft.commonFunction, 0, 176, 450, 512);
    function->onChange = [this, function] { draft.commonFunction = function->getText(); };
    text("Ресурсы в data/: пути через запятую", 480, 152, 16);
    TextField *resources = field(joinCodes(draft.resources), 480, 176, 460, 1024);
    resources->onChange = [this, resources] { draft.resources = splitCodes(resources->getText()); };
    std::vector<std::string> parts;
    for (const auto &part : draft.parts)
        parts.push_back(part.id + " · " + kindName(part.kind));
    ListBox *partList = list(parts, 0, 240, 240, 120);
    partList->select(partIndex);
    partList->onChange = [this](int value) {
        defer([this, value] {
            partIndex = value;
            editor();
        });
    };
    button("+ Часть", 0, 370, 115, [this] {
        TaskPart part;
        part.id = "part-" + std::to_string(draft.parts.size() + 1);
        while (std::find_if(draft.parts.begin(), draft.parts.end(), [&part](const TaskPart &other) {
                   return other.id == part.id;
               }) != draft.parts.end())
            part.id += 'x';
        part.prompt = "Решите уравнение";
        part.expression = "x=1";
        draft.parts.push_back(part);
        partIndex = static_cast<int>(draft.parts.size() - 1);
        editor();
    });
    button("− Часть", 125, 370, 115, [this] {
        if (draft.parts.size() == 1)
            throw std::runtime_error("Оставьте хотя бы одну часть");
        draft.parts.erase(draft.parts.begin() + partIndex);
        partIndex = 0;
        editor();
    });
    if (draft.parts.empty())
        return;
    TaskPart &part = draft.parts[partIndex];
    ListBox *type = list({"Значение", "Уравнение", "Прямая", "Парабола", "Число", "Выбор вариантов", "Выражение с x"}, 0, 430, 240, 120);
    type->select(static_cast<int>(part.kind));
    type->onChange = [this](int value) {
        draft.parts[partIndex].kind = static_cast<PartKind>(value);
    };
    button("Дополнительно", 0, 566, 240, [this] {
        clear();
        title = "Настройки части задания";
        const auto &part=draft.parts[partIndex];
        text("Привязка графика: 0.01–10",0,0,16);
        auto snap=field(std::to_string(part.graphSnap),0,26,300,20);
        text("Допуск графика: 0.000001–0.1",350,0,16);
        auto tolerance=field(std::to_string(part.graphTolerance),350,26,300,20);
        text("Округление: -1 — точное, 0–6 — знаки",0,86,16);
        auto places=field(std::to_string(part.decimalPlaces),0,112,300,2);
        text("Числовая погрешность: 0–1",350,86,16);
        auto numeric=field(std::to_string(part.answerTolerance),350,112,300,20);
        note("Округление и числовая погрешность действуют только для числового ответа.",0,170,900);
        text("Формула для ученика (необязательно)",0,215,16);
        auto display=field(part.displayExpression,0,241,900,512);
        auto steps=add(new Toggle(font,"Обязательное решение линейного уравнения",650),0,305);
        steps->setValue(part.requireSteps);
        std::string options,correct;
        for (const auto &value : part.choices) { if (!options.empty()) options+="|"; options+=value; }
        for (int value : part.correctChoices) { if (!correct.empty()) correct+=";"; correct+=std::to_string(value); }
        text("Варианты ответа через | (только для выбора вариантов)",0,365,16);
        auto choices=field(options,0,391,900,2048);
        text("Номера правильных вариантов через ;",0,451,16);
        auto correctField=field(correct,0,477,900,100);
        button("Применить",0,566,240,[this,snap,tolerance,places,numeric,display,steps,choices,correctField] {
            TaskPart &part=draft.parts[partIndex];
            double digits=readNumber(places->getText());
            if (digits < -1 || digits>6 || digits!=int(digits))
                throw std::runtime_error("Округление: целое от -1 до 6");
            std::vector<std::string> options; std::vector<int> correct;
            std::string token; std::istringstream texts(choices->getText()), numbers(correctField->getText());
            while (std::getline(texts,token,'|')) options.push_back(token);
            while (std::getline(numbers,token,';')) {
                double id=readNumber(token);
                if (id<1 || id>12 || id!=int(id)) throw std::runtime_error("Номер варианта: целое от 1 до 12");
                correct.push_back(int(id));
            }
            double step=readNumber(snap->getText()), precision=readNumber(tolerance->getText()), allowed=readNumber(numeric->getText());
            if (step<0.01 || step>10 || precision<0.000001 || precision>0.1 || allowed<0 || allowed>1)
                throw std::runtime_error("Допуск или привязка вне указанных границ");
            part.graphSnap=step; part.graphTolerance=precision; part.decimalPlaces=int(digits);
            part.answerTolerance=allowed; part.requireSteps=steps->getValue();
            part.displayExpression=display->getText(); part.choices=options; part.correctChoices=correct;
            editor();
        },true);
        button("Отмена",260,566,200,[this] { editor(); });
    });
    text("Условие выбранной части", 270, 228, 16);
    TextField *prompt = field(part.prompt, 270, 252, 670, 2048);
    prompt->onChange = [this, prompt] { draft.parts[partIndex].prompt = prompt->getText(); };
    text("Формула / уравнение", 270, 302, 16);
    TextField *expression = field(part.expression, 270, 326, 670, 512);
    FormulaView *formula = add(new FormulaView(font, 670, 90), 270, 378);
    formula->setFormula(part.expression);
    expression->onChange = [this, expression, formula] {
        draft.parts[partIndex].expression = expression->getText();
        formula->setFormula(expression->getText());
    };
    text("Коды навыков через запятую", 270, 476, 16);
    TextField *skills = field(joinCodes(part.skills), 270, 500, 390, 512);
    skills->onChange = [this, skills] {
        draft.parts[partIndex].skills = splitCodes(skills->getText());
    };
    text("Балл", 680, 476, 16);
    TextField *score = field(std::to_string(part.maxScore), 680, 500, 100, 3);
    text("Сложность", 800, 476, 16);
    TextField *difficulty =
        field(std::to_string(static_cast<int>(draft.difficulty)), 800, 500, 140, 4);
    auto readScores = [this, score, difficulty] {
        double points = readNumber(score->getText()), level = readNumber(difficulty->getText());
        if (points != static_cast<int>(points) || points < 1 || points > 100)
            throw std::runtime_error("Балл: целое от 1 до 100");
        draft.parts[partIndex].maxScore = static_cast<int>(points);
        draft.difficulty = level;
    };
    // Числа сохраняются и при переключении части, если введено допустимое значение.
    score->onChange = [this, score] {
        try {
            double value = readNumber(score->getText());
            if (value >= 1 && value <= 100 && value == static_cast<int>(value))
                draft.parts[partIndex].maxScore = static_cast<int>(value);
        } catch (...) {
        }
    };
    difficulty->onChange = [this, difficulty] {
        try {
            draft.difficulty = readNumber(difficulty->getText());
        } catch (...) {
        }
    };
    button("Сохранить", 270, 566, 190, [this, readScores] {
        readScores();
        context.school.saveDraft(draft);
        status = "Черновик сохранён";
    });
    button(
        "Опубликовать", 475, 566, 230,
        [this, readScores] {
            readScores();
            context.school.saveDraft(draft);
            context.school.publish(draft.id);
            editing = false;
            refresh();
            status = "Версия опубликована";
        },
        true);
    button("Предпросмотр", 720, 566, 220, [this, readScores] {
        readScores();
        preview();
    });
}
void BankScreen::preview() {
    clear();
    title = draft.title;
    text(draft.description, 0, 0, 20, 900, 3);
    const TaskPart &part = draft.parts[partIndex];
    text(part.prompt, 0, 95, 22, 900, 3);
    FormulaView *formula = add(new FormulaView(font, 940, 180), 0, 200);
    formula->setFormula(part.displayExpression.empty() ? part.expression : part.displayExpression);
    text("Часть " + part.id + " · " + kindName(part.kind) + " · " + std::to_string(part.maxScore) +
             " балл.",
         0, 410, 18);
    auto errors = context.school.validate(draft);
    text(errors.empty() ? "Проверка структуры пройдена" : errors.front(), 0, 460, 18, 900, 3);
    button("К редактированию", 0, 566, 280, [this] { editor(); });
}
} // namespace ui
} // namespace exam
