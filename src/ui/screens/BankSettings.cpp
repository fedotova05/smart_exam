#include "BankScreen.h"
#include <algorithm>
#include <memory>
#include <set>
namespace exam {
namespace ui {
void BankScreen::selectSkills() {
    clear();
    title = "Навыки задания";
    note("Отметьте навыки, которые проверяет выбранная часть задания.", 0, 0, 940);
    auto skills = context.school.skills();
    std::vector<std::string> rows;
    auto selected = std::make_shared<std::set<std::string>>(draft.parts[partIndex].skills.begin(),
                                                            draft.parts[partIndex].skills.end());
    for (const auto &skill : skills)
        rows.push_back(skill.name);
    auto choices = list(rows, 0, 60, 940, 420);
    choices->setRowHeight(56);
    choices->setCheckable(true);
    for (std::size_t i = 0; i < skills.size(); ++i)
        choices->setChecked(int(i), selected->count(skills[i].id) != 0);
    choices->onToggle = [selected, choices, skills](int i) {
        if (!selected->erase(skills[i].id))
            selected->insert(skills[i].id);
        choices->setChecked(i, selected->count(skills[i].id) != 0);
    };
    button(
        "Применить", 0, 550, 260,
        [this, selected] {
            if (selected->empty())
                throw std::runtime_error("Выберите хотя бы один навык");
            draft.parts[partIndex].skills.assign(selected->begin(), selected->end());
            editor();
        },
        true);
    button("Отмена", 280, 550, 200, [this] { editor(); });
    focusWidget(choices);
}
void BankScreen::answerSettings(bool reset) {
    if (reset) {
        answerDraft = draft.parts[partIndex];
        choiceFirst = 0;
    }
    clear();
    title = "Настройка ответа";
    text(kindName(answerDraft.kind), 0, 0, 23);
    text("Формула, которую увидит ученик", 0, 52, 16);
    auto display = field(answerDraft.displayExpression, 0, 80, 940, 512);
    display->setPlaceholder("Оставьте пустым, чтобы показывать формулу из задания");
    display->onChange = [this, display] { answerDraft.displayExpression = display->getText(); };
    std::function<void()> validate = [] {};
    if (answerDraft.kind == PartKind::Choice) {
        note("Введите варианты и отметьте все правильные ответы.", 0, 144, 940);
        if (answerDraft.choices.empty())
            answerDraft.choices.resize(2);
        if (choiceFirst >= int(answerDraft.choices.size()))
            choiceFirst = 0;
        for (int i = choiceFirst; i < std::min(choiceFirst + 4, int(answerDraft.choices.size()));
             ++i) {
            float y = 190 + (i - choiceFirst) * 72.f;
            auto correct = add(new Toggle(font, std::to_string(i + 1), 90), 0, y);
            correct->setValue(std::find(answerDraft.correctChoices.begin(),
                                        answerDraft.correctChoices.end(),
                                        i + 1) != answerDraft.correctChoices.end());
            correct->onChange = [this, correct, i](bool) {
                auto &values = answerDraft.correctChoices;
                values.erase(std::remove(values.begin(), values.end(), i + 1), values.end());
                if (correct->getValue())
                    values.push_back(i + 1);
            };
            auto value = field(answerDraft.choices[i], 110, y, 830, 512);
            value->setPlaceholder("Текст варианта");
            value->onChange = [this, value, i] { answerDraft.choices[i] = value->getText(); };
        }
        button("+ Вариант", 0, 488, 200, [this] {
            answerDraft.choices.push_back("");
            choiceFirst = (int(answerDraft.choices.size()) - 1) / 4 * 4;
            answerSettings(false);
        })->setEnabled(answerDraft.choices.size() < 12);
        button("Убрать последний", 215, 488, 245, [this] {
            int n = int(answerDraft.choices.size());
            answerDraft.choices.pop_back();
            auto &v = answerDraft.correctChoices;
            v.erase(std::remove(v.begin(), v.end(), n), v.end());
            answerSettings(false);
        })->setEnabled(answerDraft.choices.size() > 2);
        if (choiceFirst > 0)
            button("← Назад", 600, 488, 150, [this] {
                choiceFirst -= 4;
                answerSettings(false);
            });
        if (choiceFirst + 4 < int(answerDraft.choices.size()))
            button("Далее →", 770, 488, 170, [this] {
                choiceFirst += 4;
                answerSettings(false);
            });
        validate = [this] {
            for (const auto &value : answerDraft.choices)
                if (value.find_first_not_of(" \t\r\n") == std::string::npos)
                    throw std::runtime_error("Заполните все варианты ответа");
            if (answerDraft.correctChoices.empty())
                throw std::runtime_error("Отметьте правильный ответ");
        };
    } else if (answerDraft.kind == PartKind::Line || answerDraft.kind == PartKind::Parabola) {
        text("Шаг привязки точки", 0, 170, 18);
        auto snap = field(std::to_string(answerDraft.graphSnap), 0, 210, 420, 20);
        note("От 0.01 до 10 единиц координатной сетки.", 0, 270, 420);
        text("Допустимое отклонение", 490, 170, 18);
        auto tolerance = field(std::to_string(answerDraft.graphTolerance), 490, 210, 450, 20);
        note("От 0.000001 до 0.1. Меньше — точнее проверка.", 490, 270, 450);
        validate = [this, snap, tolerance] {
            double step = readNumber(snap->getText()), precision = readNumber(tolerance->getText());
            if (step < 0.01 || step > 10 || precision < 0.000001 || precision > 0.1)
                throw std::runtime_error("Значения вне указанных границ");
            answerDraft.graphSnap = step;
            answerDraft.graphTolerance = precision;
        };
    } else if (answerDraft.kind == PartKind::Number) {
        text("Округление ответа", 0, 170, 18);
        auto rounding =
            list({"Без округления", "До целого числа", "1 знак после запятой",
                  "2 знака после запятой", "3 знака после запятой", "4 знака после запятой",
                  "5 знаков после запятой", "6 знаков после запятой"},
                 0, 210, 450, 240);
        rounding->select(answerDraft.decimalPlaces + 1);
        rounding->onChange = [this](int i) { answerDraft.decimalPlaces = i - 1; };
        text("Допустимая погрешность", 490, 170, 18);
        auto tolerance = field(std::to_string(answerDraft.answerTolerance), 490, 210, 450, 20);
        note("От 0 до 1. Ноль — ответ должен совпасть точно.", 490, 270, 450);
        validate = [this, tolerance] {
            double value = readNumber(tolerance->getText());
            if (value < 0 || value > 1)
                throw std::runtime_error("Погрешность: число от 0 до 1");
            answerDraft.answerTolerance = value;
        };
    } else if (answerDraft.kind == PartKind::Equation) {
        auto steps = add(new Toggle(font, "Требовать решение по шагам", 600), 0, 180);
        steps->setValue(answerDraft.requireSteps);
        steps->onChange = [this, steps](bool) { answerDraft.requireSteps = steps->getValue(); };
        note("Пошаговая проверка доступна только для линейных уравнений. Для остальных оставьте "
             "этот параметр выключенным.",
             0, 250, 800);
    } else {
        note("Ответ проверяется как точное математическое выражение. Дополнительные параметры не "
             "нужны.",
             0, 180, 850);
    }
    button(
        "Применить", 0, 566, 260,
        [this, validate] {
            validate();
            draft.parts[partIndex] = answerDraft;
            editor();
        },
        true);
    button("Отмена", 280, 566, 200, [this] { editor(); });
}
} // namespace ui
} // namespace exam
