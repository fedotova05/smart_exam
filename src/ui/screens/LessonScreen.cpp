#include "LessonScreen.h"
#include "ui/ImageView.h"
#include "math/checkers/EquationChecker.h"
#include "ui/widgets/TextPanel.h"
#include <set>
#include <sstream>
namespace exam {
namespace ui {
void LessonScreen::route() {
    const auto progress = context.school.progress(context.profile);
    note("Задание " + std::to_string(context.current.attempt.position + 1) + " из " +
             std::to_string(progress.totalTasks),
         0, -24, 500);
    const auto lesson = progress.lesson;
    button(lesson.settings.resumePolicy == ResumePolicy::ActiveTime ? "Пауза" : "К занятию", 760,
           -110, 180, [this, lesson] {
               if (!lesson.closed && lesson.settings.resumePolicy == ResumePolicy::ActiveTime)
                   context.school.pauseLesson();
               context.page = "home";
           });
}
void LessonScreen::send(bool finalAnswer, const std::string &operation,
                        const std::string &operand) {
    const TaskPart &part = context.current.task.parts[partIndex];
    CheckResult result =
        context.school.submit(context.current.attempt.id, part.id, context.drafts[part.id],
                              finalAnswer, operation, operand);
    feedback = result.message;
    if (result.errorPosition >= 0)
        feedback += " · позиция " + std::to_string(result.errorPosition + 1);
    if (finalAnswer)
        answerSubmitted = true;
    refresh();
    status = feedback;
    if (finalAnswer && result.status == CheckStatus::Incorrect &&
        ((!focusCanvas && context.school.canSupport(attempt)) ||
         !context.school.recommendedLectures(attempt).empty()))
        supportChoice();
}
void LessonScreen::supportChoice() {
    clear();
    focusCanvas = false;
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
        true)
        ->setEnabled(context.school.canSupport(attempt));
    button("Попробовать ещё раз", 530, 460, 350, [this] { refresh(); });
    if (!context.school.recommendedLectures(attempt).empty())
        button("Почитать лекцию", 0, 525, 500, [this] { lectureChoices(); });
}
void LessonScreen::lectureChoices() {
    auto values = context.school.recommendedLectures(attempt);
    clear();
    focusCanvas = false;
    title = "Материалы к заданию";
    note("Сначала показаны лекции по текущему навыку, затем — по необходимым основам.", 0, 0, 940);
    std::vector<std::string> names;
    for (auto &v : values)
        names.push_back(v.title);
    auto choices = list(names, 0, 60, 940, 420);
    choices->setRowHeight(64);
    button(
        "Прочитать", 0, 540, 300,
        [this, choices, values] {
            if (choices->selection() < 0)
                throw std::runtime_error("Выберите лекцию");
            showLecture(values[choices->selection()].id);
        },
        true)
        ->setEnabled(!values.empty());
    button("← К заданию", 640, 540, 300, [this] { refresh(); });
}
void LessonScreen::showLecture(const std::string &id) {
    context.school.readLecture(id, attempt);
    auto value = context.school.lecture(id);
    clear();
    focusCanvas = false;
    title = value.title;
    note("Чтение учтено как помощь. Ответ и текущая попытка сохраняются.", 0, 0, 940);
    add(new TextPanel(font, value.body.empty() ? "Материал находится во вложении." : value.body,
                      940, 430),
        0, 50);
    button("← К материалам", 0, 540, 280, [this] { lectureChoices(); });
    if (!value.attachment.empty())
        button("Открыть PDF / Word", 300, 540, 300,
               [this, id] { context.school.openLectureFile(id); });
    button("К заданию →", 640, 540, 300, [this] { refresh(); }, true);
}
void LessonScreen::refresh() {
    clear();
    focusCanvas = false;
    title = "Решение задания";
    if (!context.current.available) {
        text(context.current.reason.empty() ? "Начните занятие на странице «Занятие»."
                                            : context.current.reason,
             0, 30, 24);
        button("К результатам", 0, 110, 220, [this] {
            context.historyLesson = context.school.currentLesson(context.profile).id;
            context.historyAttempt.clear();
            context.page = "results";
        });
        return;
    }
    const TaskVersion &task = context.current.task;
    if (attempt != context.current.attempt.id) {
        attempt = context.current.attempt.id;
        partIndex = 0;
        stepMode = false;
        answerSubmitted = false;
        feedback.clear();
        detail.clear();
        stepEquation.clear();
        stepOperand.clear();
        stepOperation = 0;
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
    if (context.current.attempt.finished) {
        focusCanvas = true;
        setContentWidth(720);
        focusedResult();
        return;
    }
    if (task.parts.size() == 1 && part.kind != PartKind::Line && part.kind != PartKind::Parabola &&
        part.kind != PartKind::Choice) {
        focusCanvas = true;
        setContentWidth(720);
        if (context.current.attempt.finished)
            focusedResult();
        else if (!detail.empty())
            focusedDetails();
        else
            focusedTask();
        return;
    }
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
    FormulaView *condition = add(new FormulaView(headingFace(), 600,
                                                 graphPart                       ? 90
                                                 : part.kind == PartKind::Choice ? 80
                                                                                 : 150),
                                 0, 118);
    condition->setVisible(part.kind != PartKind::ShortAnswer || !part.displayExpression.empty());
    condition->setFeatured(true);
    condition->setFormula(part.displayExpression.empty() ? part.expression
                                                         : part.displayExpression);
    if (part.kind == PartKind::Choice) {
        std::vector<std::string> options;
        for (std::size_t i = 0; i < part.choices.size(); ++i)
            options.push_back(std::to_string(i + 1) + ". " + part.choices[i]);
        auto choices = list(options, 0, 210, 600, 280);
        choices->setRowHeight(60);
        choices->setCheckable(true);
        auto selected = std::make_shared<std::set<int>>();
        std::istringstream values(context.drafts[part.id].text);
        std::string token;
        while (std::getline(values, token, ';')) {
            try {
                selected->insert(std::stoi(token));
            } catch (...) {
            }
        }
        for (std::size_t i = 0; i < part.choices.size(); ++i)
            choices->setChecked(int(i), selected->count(int(i) + 1) != 0);
        choices->onToggle = [this, choices, selected, part](int i) {
            if (!selected->erase(i + 1))
                selected->insert(i + 1);
            choices->setChecked(i, selected->count(i + 1) != 0);
            std::string value;
            for (int n : *selected) {
                if (!value.empty())
                    value += ";";
                value += std::to_string(n);
            }
            context.drafts[part.id].text = value;
            context.school.saveAnswerDraft(attempt, part.id, context.drafts[part.id]);
            status.clear();
        };
        note("Отметьте все правильные варианты. Пробел — поставить или снять флажок.", 0, 510, 600);
        text("Выберите ответы слева и отправьте их на проверку.", 630, 180, 19, 310, 4);
        button("Отправить ответ", 0, 566, 230, [this] { send(true); }, true);
        focusWidget(choices);
    }
    if (graphPart) {
        GraphView *graph = add(new GraphView(font, 600, 270), 0, 220);
        graph->setAnswer(part.kind, context.drafts[part.id].graph);
        graph->setSnapStep(part.graphSnap);
        graph->onChange = [this, graph, part] {
            context.drafts[part.id].graph = graph->getAnswer();
            context.school.saveAnswerDraft(attempt, part.id, context.drafts[part.id]);
        };
        Toggle *snap = add(new Toggle(font, "По сетке", 180), 0, 503);
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
            button("Построить", 415, 503, 185, [this, graph, a, b, c, part] {
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
        text(graphPart && part.kind == PartKind::Line
                 ? "Поставьте две точки щелчком по сетке. Их можно перетаскивать. Колесо меняет "
                   "масштаб, правая кнопка сдвигает поле."
                 : "Введите a, b, c для y = ax² + bx + c и нажмите «Построить». Колесо меняет "
                   "масштаб, правая кнопка сдвигает поле.",
             630, 170, 16, 310, 5);
    } else if (part.kind != PartKind::Choice) {
        text("Итоговый ответ", 0, 296, 16);
        TextField *answer = field(context.drafts[part.id].text, 0, 324, 420, 512);
        answer->setPlaceholder("Введите ответ");
        answer->onEnter = [this] { defer([this] { send(true); }); };
        button("Проверить →", 440, 324, 160, [this] { send(true); }, true);
        FormulaView *preview = add(new FormulaView(headingFace(), 600, 85), 0, 425);
        preview->setFormula(part.kind == PartKind::ShortAnswer ? "" : answer->getText());
        answer->onChange = [this, answer, preview, part] {
            context.drafts[part.id].text = answer->getText();
            context.school.saveAnswerDraft(attempt, part.id, context.drafts[part.id]);
            preview->setFormula(part.kind == PartKind::ShortAnswer ? "" : answer->getText());
        };
        const char *inserts[] = {"x", "^2", "sqrt(", "(", ")", "/"};
        for (int i = 0; part.kind != PartKind::ShortAnswer && i < 6; ++i) {
            std::string token = inserts[i];
            button(token, i * 60.f, 375, 54, [answer, token] { answer->insert(token); });
        }
        text(part.kind == PartKind::ShortAnswer
                 ? "Введите только ответ. Порядок цифр важен; десятичная дробь — через точку или "
                   "запятую."
                 : (part.kind == PartKind::Equation
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
            if (part.kind == PartKind::ShortAnswer)
                instruction = "Запишите слово, число или последовательность. Регистр и пробелы по "
                              "краям не учитываются.";
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
    Button *hint = button(
        context.settings.helpAllowed ? "Нужна подсказка?" : "Без подсказок",
        (graphPart || part.kind == PartKind::Choice) ? 245 : 0, 574, 180, [this, part] {
            context.school.help(attempt, part.id);
            status =
                part.kind == PartKind::ShortAnswer
                    ? "Перечитайте вопрос. Запишите только ответ в нужном порядке, без пояснений."
                : part.kind == PartKind::Equation
                    ? "Для квадратного уравнения используйте D = b² − 4ac. Подставьте найденные "
                      "корни для проверки. Для линейного выполняйте одно действие с обеими частями."
                : (part.kind == PartKind::Expression || part.kind == PartKind::Number)
                    ? "Сначала действия в скобках, затем умножение и деление."
                : part.kind == PartKind::Choice ? "Проверьте каждый вариант отдельно. В ответе "
                                                  "нужны номера всех верных вариантов."
                : part.kind == PartKind::Polynomial
                    ? "Раскройте скобки, затем сложите коэффициенты при одинаковых степенях x."
                    : "Прямая задаётся двумя различными точками; у параболы коэффициент a не "
                      "равен нулю.";
        });
    hint->setEnabled(context.settings.helpAllowed);
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
}
void LessonScreen::resources(int index) {
    // Сначала читаем ресурс: ошибка не должна разрушать текущий экран.
    std::unique_ptr<ImageView> image(new ImageView(font, context.current.task.resources.at(index)));
    clear();
    focusCanvas = false;
    title = "Рисунок к заданию";
    std::vector<std::string> names;
    for (std::size_t i = 0; i < context.current.task.resources.size(); ++i)
        names.push_back("Рисунок / формула " + std::to_string(i + 1));
    ListBox *files = list(names, 0, 0, 940, 80);
    files->select(index);
    files->onChange = [this](int value) { defer([this, value] { resources(value); }); };
    add(image.release(), 0, 95);
    button("К заданию", 0, 566, 240, [this] { refresh(); });
}
} // namespace ui
} // namespace exam
