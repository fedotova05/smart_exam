#include "LessonScreen.h"
#include "ui/ImageView.h"
#include "math/checkers/EquationChecker.h"
#include "ui/widgets/TextPanel.h"
namespace exam {
namespace ui {
void LessonScreen::route() {
    const SessionRecord lesson = context.school.latestLesson(context.profile);
    const auto attempts = context.school.lessonAttempts(lesson.id);
    int total = lesson.settings.mode == SessionMode::Fixed
                    ? std::min(lesson.settings.taskLimit, static_cast<int>(lesson.tasks.size()))
                    : lesson.settings.taskLimit;
    if (lesson.settings.training)
        total = context.school.progress(context.profile).totalTasks;
    int completed = 0;
    for (const auto &item : attempts)
        if (item.finished)
            ++completed;
    note("ВАШ МАРШРУТ", -156, 0, 140);
    int position = context.current.attempt.position;
    int first = std::max(0, position - 2);
    for (int i = first; i < std::min(total, first + 5); ++i) {
        float y = 42 + (i - first) * 78.f;
        bool finished = i < static_cast<int>(attempts.size()) && attempts[i].finished;
        if (i == position)
            surface(-156, y - 8, 140, 70, Theme::pale);
        text(std::to_string(i + 1) + (finished        ? "  Готово"
                                      : i == position ? "  Сейчас"
                                                      : "  Впереди"),
             -148, y, 14, 125, 1);
        std::string name = "Подбор по навыкам";
        if (lesson.settings.mode == SessionMode::Fixed)
            name = context.school.task(lesson.tasks[i]).title;
        else if (i < static_cast<int>(attempts.size()))
            name = context.school.task(attempts[i].taskId).title;
        text(name, -148, y + 24, 12, 125, 2);
    }
    surface(-156, 455, 140, 1, Theme::line);
    text(std::to_string(completed) + " из " + std::to_string(total), -156, 477, 23, 140, 1);
    note("заданий завершено", -156, 515, 140);
    button("К занятию", -156, 574, 140, [this] { context.page = "home"; });
}
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
    if (finalAnswer && result.status == CheckStatus::Incorrect &&
        context.school.canSupport(attempt))
        supportChoice();
}
void LessonScreen::supportChoice() {
    clear();
    title = "Ответ пока неверный";
    heading("Как продолжим?", 0, 30, 32);
    text(
        "Можно исправить ответ самостоятельно или проверить навыки, которые нужны для этой задачи.",
        0, 100, 22, 900, 3);
    text("Разбор использует отдельные упражнения из банка. Сначала уточним затруднение, "
         "затем потренируем нужный навык и вернёмся к исходному заданию.",
         0, 210, 20, 900, 4);
    note("По одному неверному ответу программа не определяет причину ошибки. "
         "Ваш ответ сохранён. При переходе к разбору текущая попытка будет завершена.",
         0, 340, 900);
    button(
        "Разобраться на простых заданиях →", 0, 460, 500,
        [this] {
            context.current = context.school.startSupport(attempt);
            refresh();
        },
        true);
    button("Попробовать ещё раз", 530, 460, 350, [this] { refresh(); });
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
        stepMode = false;
        context.drafts.clear();
        for (const auto &response : context.school.responses(attempt))
            if (response.finalAnswer)
                context.drafts[response.partId] = response.answer;
        for (const auto &draft : context.school.answerDrafts(attempt))
            context.drafts[draft.first] = draft.second;
    }
    if (partIndex >= static_cast<int>(task.parts.size()))
        partIndex = 0;
    const TaskPart &part = task.parts[partIndex];
    bool linearSteps = part.kind == PartKind::Equation && isLinearEquation(part.expression);
    if (!linearSteps)
        stepMode = false;
    title = task.title;
    route();
    std::string conditionText =
        task.description.empty() ? part.prompt : task.description + "\n" + part.prompt;
    if (context.settings.training)
        conditionText = context.current.reason + "\n" + conditionText;
    if (part.requireSteps)
        conditionText += "\nНужно решение по шагам до x = число.";
    if (part.decimalPlaces >= 0)
        conditionText +=
            "\nОкруглите до " + std::to_string(part.decimalPlaces) + " знаков после запятой.";
    if (part.answerTolerance > 0)
        conditionText += "\nДопустимая погрешность: " + std::to_string(part.answerTolerance);
    add(new TextPanel(font, conditionText, 600, 110), 0, 0);
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
    condition->setFormula(part.displayExpression.empty() ? part.expression
                                                         : part.displayExpression);
    if (part.kind == PartKind::Choice) {
        std::vector<std::string> options;
        for (std::size_t i = 0; i < part.choices.size(); ++i)
            options.push_back(std::to_string(i + 1) + ". " + part.choices[i]);
        list(options, 0, 118, 600, 150);
    }
    if (graphPart) {
        GraphView *graph = add(new GraphView(font, 600, 270), 0, 220);
        graph->setAnswer(part.kind, context.drafts[part.id].graph);
        graph->setSnapStep(part.graphSnap);
        graph->onChange = [this, graph, part] {
            context.drafts[part.id].graph = graph->getAnswer();
            context.school.saveAnswerDraft(attempt, part.id, context.drafts[part.id]);
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
                context.school.saveAnswerDraft(attempt, part.id, context.drafts[part.id]);
            });
        } else {
            button("Сбросить точки", 190, 503, 180, [this, graph, part] {
                context.drafts[part.id].graph = GraphAnswer();
                context.school.saveAnswerDraft(attempt, part.id, context.drafts[part.id]);
                graph->setAnswer(part.kind, GraphAnswer());
            });
            button("Исходный вид", 380, 503, 220, [graph] { graph->resetView(); });
        }
        text("Колесо — масштаб; правая кнопка — сдвиг. Прямая: две точки; парабола: y = ax² + bx + "
             "c.",
             630, 170, 16, 310, 5);
    } else {
        text("Итоговый ответ", 0, 296, 16);
        TextField *answer = field(context.drafts[part.id].text, 0, 324, 420, 512);
        answer->setPlaceholder("Введите ответ");
        button("Проверить →", 440, 324, 160, [this] { send(true); }, true);
        FormulaView *preview = add(new FormulaView(headingFace(), 600, 85), 0, 425);
        preview->setFormula(answer->getText());
        answer->onChange = [this, answer, preview, part] {
            context.drafts[part.id].text = answer->getText();
            context.school.saveAnswerDraft(attempt, part.id, context.drafts[part.id]);
            preview->setFormula(answer->getText());
        };
        const char *inserts[] = {"x", "^2", "sqrt(", "(", ")", "/"};
        for (int i = 0; i < 6; ++i) {
            std::string token = inserts[i];
            button(token, i * 60.f, 375, 54, [answer, token] { answer->insert(token); });
        }
        text((part.kind == PartKind::Equation
                  ? "Корни: 2; 3. Нет корней: none. Все числа: all.\n"
                  : "Число: -3. Дробь: 1/2. Квадратный корень: sqrt(5).\n") +
                 std::string("Предпросмотр: колесо — прокрутка, Ctrl + колесо — масштаб."),
             0, 522, 13, 600, 2);
        if (linearSteps)
            button(stepMode            ? "Скрыть шаги ↑"
                   : part.requireSteps ? "Шаги (обязательно) ↓"
                                       : "Шаги (необязательно) ↓",
                   630, 166, 310, [this] {
                       stepMode = !stepMode;
                       refresh();
                   });
        if (linearSteps && stepMode) {
            TextField *nextEquation = field("", 630, 213, 310, 512);
            nextEquation->setPlaceholder("Новое уравнение: 2*x=8");
            ListBox *operation = list(
                {"Прибавить", "Вычесть", "Умножить", "Разделить", "Поменять стороны", "Упростить"},
                630, 265, 310, 80);
            TextField *operand = field("", 630, 355, 100, 100);
            operand->setPlaceholder("Число");
            button("Проверить шаг", 740, 355, 200, [this, operation, operand, nextEquation, part] {
                const char *operations[] = {"add",    "subtract", "multiply",
                                            "divide", "swap",     "simplify"};
                AnswerPayload answer;
                answer.text = nextEquation->getText();
                CheckResult result =
                    context.school.submit(attempt, part.id, answer, false,
                                          operations[operation->selection()], operand->getText());
                refresh();
                status = result.message;
            });
        } else {
            note("КАК ОТВЕТИТЬ", 630, 236, 310);
            std::string instruction =
                "Вычислите значение и введите его слева. Дробь можно записать через /.";
            if (part.kind == PartKind::Choice)
                instruction =
                    "Введите номера выбранных вариантов через ;. Например: 1;3. Порядок не важен.";
            if (part.kind == PartKind::Polynomial)
                instruction =
                    "Введите выражение с x. Например: x^2+2*x+1. Равносильные записи принимаются.";
            if (linearSteps)
                instruction = part.requireSteps ? "В разделе шагов преобразуйте уравнение до x = "
                                                  "число. Затем отправьте итоговый ответ слева."
                                                : "Введите значение x слева. По желанию проверьте "
                                                  "преобразования в разделе шагов.";
            else if (part.kind == PartKind::Equation)
                instruction =
                    "Решите на бумаге, например через дискриминант. Введите все корни слева "
                    "через точку с запятой. Шаги здесь не проверяются.";
            text(instruction, 630, 270, 16, 310, 6);
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
                context.school.finish(attempt);
                context.current.attempt.finished = true;
                const LessonProgress progress = context.school.progress(context.profile);
                if (!progress.lesson.settings.training &&
                    progress.finishedTasks >= progress.totalTasks)
                    context.school.closeLesson();
                refresh();
            });
    });
    if (context.current.attempt.finished) {
        clear();
        route();
        title = "Задание завершено";
        const LessonProgress progress = context.school.progress(context.profile);
        for (const auto &entry : context.school.history(context.profile))
            if (entry.attemptId == attempt) {
                note("РЕЗУЛЬТАТ ЭТОГО ЗАДАНИЯ", 0, 25, 450);
                text(scoreText(entry.result, entry.maximumScore), 0, 67, 34, 450, 2);
                note(entry.result.provisional && entry.result.visible
                         ? "Не все части проверены. Посмотрите ответы в истории."
                         : "Баллы за одно задание не означают освоение всей темы.",
                     0, 165, 440);
                note("Подсказок: " + std::to_string(entry.help) +
                         (entry.hadErrors ? ". В решении были ошибки." : "."),
                     0, 242, 440);
            }
        note("ХОД ЗАНЯТИЯ", 500, 25, 440);
        heading(std::to_string(progress.finishedTasks) + " из " +
                    std::to_string(progress.totalTasks),
                500, 67, 38);
        progressBar(progress.totalTasks == 0 ? 0.f
                                             : float(progress.finishedTasks) / progress.totalTasks,
                    500, 130, 440);
        text("Баллы: " + lessonScore(progress), 500, 166, 24, 440, 1);
        note(lessonNote(progress), 500, 215, 440);
        surface(0, 320, 940, 1, Theme::line);
        note("Навыки оцениваются по серии самостоятельных решений. Повторные проверки одного "
             "задания не создают новые наблюдения.",
             0, 350, 900);
        if (progress.lesson.settings.training)
            text(context.current.reason, 0, 290, 16, 940, 1);
        button(
            progress.lesson.closed ? "К итогам занятия →" : "Следующее задание →", 0, 440, 300,
            [this] {
                if (context.school.latestLesson(context.profile).closed) {
                    context.current = NextTask();
                    context.page = "results";
                    return;
                }
                context.current = context.school.next();
                refresh();
            },
            true);
        button("Посмотреть навыки", 320, 440, 280, [this] { context.page = "progress"; });
        if (context.school.canSupport(attempt))
            button("Разобрать ошибку", 620, 440, 300, [this] { supportChoice(); });
        if (!progress.lesson.closed)
            button("Закончить раньше", 0, 510, 300, [this] {
                ask("Завершить занятие раньше? Невыполненные задания останутся в плане.", [this] {
                    context.school.closeLesson();
                    context.current = NextTask();
                    context.page = "results";
                });
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
