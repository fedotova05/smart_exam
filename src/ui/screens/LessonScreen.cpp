#include "LessonScreen.h"
#include "ui/ImageView.h"
#include "ui/widgets/TextPanel.h"
namespace exam {
namespace ui {
void LessonScreen::send(bool finalAnswer, const std::string &operation,
                        const std::string &operand) {
    const TaskPart &part = context.current.task.parts[partIndex];
    CheckResult result =
        context.school.submit(context.current.attempt.id, part.id, context.drafts[part.id],
                              finalAnswer, operation, operand);
    refresh();
    status = result.message;
    if (result.errorPosition >= 0)
        status += " · позиция " + std::to_string(result.errorPosition + 1);
}
void LessonScreen::refresh() {
    clear();
    title = "Решение задания";
    if (!context.current.available) {
        text(context.current.reason.empty() ? "Начните занятие на странице «Занятие»."
                                            : context.current.reason,
             0, 30, 24);
        button("К результатам", 0, 110, 220, [this] { context.page = "results"; });
        return;
    }
    const TaskVersion &task = context.current.task;
    if (attempt != context.current.attempt.id) {
        attempt = context.current.attempt.id;
        partIndex = 0;
        stepMode = context.settings.guided;
        context.drafts.clear();
        for (const auto &response : context.school.responses(attempt))
            context.drafts[response.partId] = response.answer;
    }
    if (partIndex >= static_cast<int>(task.parts.size()))
        partIndex = 0;
    const TaskPart &part = task.parts[partIndex];
    title = task.title;
    add(new TextPanel(
            font, task.description.empty() ? part.prompt : task.description + "\n" + part.prompt,
            600, 110),
        0, 0);
    std::vector<std::string> parts;
    for (const auto &item : task.parts)
        parts.push_back(item.id + " · " + kindName(item.kind) + " · " +
                        std::to_string(item.maxScore) + " б.");
    note("ЧАСТИ ЗАДАНИЯ", 630, 0, 310);
    ListBox *partList = list(parts, 630, 32, 310, 120);
    partList->select(partIndex);
    partList->onChange = [this](int index) {
        defer([this, index] {
            partIndex = index;
            refresh();
        });
    };
    bool graphPart = part.kind == PartKind::Line || part.kind == PartKind::Parabola;
    FormulaView *condition = add(new FormulaView(headingFace(), 600, graphPart ? 90 : 150), 0, 118);
    condition->setFeatured(true);
    condition->setFormula(part.expression);
    if (graphPart) {
        GraphView *graph = add(new GraphView(font, 600, 270), 0, 220);
        graph->setAnswer(part.kind, context.drafts[part.id].graph);
        graph->setSnapStep(part.graphSnap);
        graph->onChange = [this, graph, part] {
            context.drafts[part.id].graph = graph->getAnswer();
        };
        Toggle *snap = add(new Toggle(font, "Привязка", 180), 0, 503);
        snap->setValue(true);
        snap->onChange = [graph](bool value) { graph->setSnap(value); };
        if (part.kind == PartKind::Parabola) {
            const auto &coefficients = context.drafts[part.id].graph.coefficients;
            TextField *a = field(coefficients.size() == 3 ? std::to_string(coefficients[0]) : "",
                                 190, 503, 65, 20);
            TextField *b = field(coefficients.size() == 3 ? std::to_string(coefficients[1]) : "",
                                 265, 503, 65, 20);
            TextField *c = field(coefficients.size() == 3 ? std::to_string(coefficients[2]) : "",
                                 340, 503, 65, 20);
            a->setPlaceholder("a");
            b->setPlaceholder("b");
            c->setPlaceholder("c");
            button("Задать a, b, c", 415, 503, 185, [this, graph, a, b, c, part] {
                GraphAnswer answer;
                answer.coefficients = {readNumber(a->getText()), readNumber(b->getText()),
                                       readNumber(c->getText())};
                if (answer.coefficients[0] == 0)
                    throw std::runtime_error("Для параболы a не равно нулю");
                graph->setAnswer(part.kind, answer);
                context.drafts[part.id].graph = answer;
            });
        } else {
            button("Сбросить точки", 190, 503, 180, [this, graph, part] {
                context.drafts[part.id].graph = GraphAnswer();
                graph->setAnswer(part.kind, GraphAnswer());
            });
            button("Исходный вид", 380, 503, 220, [graph] { graph->resetView(); });
        }
        text("Колесо — масштаб; правая кнопка — сдвиг. Прямая: две точки; парабола: y = ax² + bx + "
             "c.",
             630, 170, 16, 310, 5);
    } else {
        text(stepMode ? "Новое уравнение или итоговый ответ" : "Ваш ответ", 0, 296, 16);
        TextField *answer = field(context.drafts[part.id].text, 0, 324, 420, 512);
        answer->setPlaceholder("Введите ответ");
        button("Отправить  →", 440, 324, 160, [this] { send(true); }, true);
        FormulaView *preview = add(new FormulaView(headingFace(), 600, 85), 0, 425);
        preview->setFormula(answer->getText());
        answer->onChange = [this, answer, preview, part] {
            context.drafts[part.id].text = answer->getText();
            preview->setFormula(answer->getText());
        };
        const char *inserts[] = {"x", "^2", "sqrt(", "(", ")", "/"};
        for (int i = 0; i < 6; ++i) {
            std::string token = inserts[i];
            button(token, i * 60.f, 375, 54, [answer, token] { answer->insert(token); });
        }
        text("Корни: 2; 3. Нет корней: none. Все числа: all.\nПредпросмотр: колесо — прокрутка, "
             "Ctrl + колесо — масштаб.",
             0, 522, 13, 600, 2);
        if (part.kind == PartKind::Equation)
            button(stepMode ? "Скрыть действия ↑" : "Решение по шагам ↓", 630, 166, 310, [this] {
                stepMode = !stepMode;
                refresh();
            });
        if (part.kind == PartKind::Equation && stepMode) {
            ListBox *operation = list(
                {"Прибавить", "Вычесть", "Умножить", "Разделить", "Поменять стороны", "Упростить"},
                630, 213, 310, 120);
            TextField *operand = field("", 630, 345, 100, 100);
            operand->setPlaceholder("Число");
            button("Отправить шаг", 740, 345, 200, [this, operation, operand] {
                const char *operations[] = {"add",    "subtract", "multiply",
                                            "divide", "swap",     "simplify"};
                send(false, operations[operation->selection()], operand->getText());
            });
        } else {
            note("ПОРЯДОК РАБОТЫ", 630, 236, 310);
            note("Введите ответ и отправьте его на проверку. Завершите задание, когда готовы "
                 "перейти дальше.",
                 630, 269, 300);
        }
    }
    note("ИСТОРИЯ ОТВЕТОВ", 630, 411, 310);
    std::vector<std::string> history;
    for (const auto &response : context.school.responses(attempt))
        if (response.partId == part.id) {
            std::string value = graphPart ? "График" : response.answer.text;
            history.push_back((response.finalAnswer ? "Ответ: " : "Шаг: ") + value + " · " +
                              response.result.message);
        }
    list(history, 630, 443, 310, 98);
    if (graphPart)
        button("Отправить ответ", 0, 566, 230, [this] { send(true); }, true);
    button("Нужна подсказка?", graphPart ? 245 : 0, 574, 180, [this, part] {
        context.school.help(attempt, part.id);
        status = part.kind == PartKind::Equation
                     ? "Выполняйте одно действие с обеими частями уравнения. Подставьте найденные "
                       "корни для проверки."
                 : part.kind == PartKind::Expression
                     ? "Сначала действия в скобках, затем умножение и деление."
                     : "Прямая задаётся двумя различными точками; у параболы коэффициент a не "
                       "равен нулю.";
    });
    if (!task.resources.empty())
        button("Рисунки", 415, 566, 185, [this] { resources(0); });
    button("Завершить задание →", 630, 574, 310, [this] {
        ask("Завершить задание? Будут учтены последние отправленные ответы по всем частям.",
            [this] {
                AttemptResult result = context.school.finish(attempt);
                context.current.attempt.finished = true;
                refresh();
                status = resultText(result);
            });
    });
    if (context.current.attempt.finished) {
        clear();
        title = "Задание завершено";
        for (const auto &entry : context.school.history(context.profile))
            if (entry.attemptId == attempt)
                text(resultText(entry.result), 0, 30, 22, 900, 3);
        button(
            "Следующее задание", 0, 150, 280,
            [this] {
                context.current = context.school.next();
                refresh();
            },
            true);
        button("Завершить занятие", 300, 150, 280, [this] {
            context.school.closeLesson();
            context.current = NextTask();
            context.page = "results";
        });
    }
}
void LessonScreen::resources(int index) {
    // Сначала читаем ресурс: ошибка не должна разрушать текущий экран.
    std::unique_ptr<ImageView> image(new ImageView(font, context.current.task.resources.at(index)));
    clear();
    title = "Рисунок к заданию";
    ListBox *files = list(context.current.task.resources, 0, 0, 940, 80);
    files->select(index);
    files->onChange = [this](int value) { defer([this, value] { resources(value); }); };
    add(image.release(), 0, 95);
    button("К заданию", 0, 566, 240, [this] { refresh(); });
}
} // namespace ui
} // namespace exam
