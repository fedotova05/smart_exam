#include "LessonScreen.h"
#include "ui/widgets/TextPanel.h"
#include "math/checkers/EquationChecker.h"
namespace exam {
namespace ui {
void LessonScreen::handle(const sf::Event &event, sf::Vector2f mouse) {
    if (focusCanvas && !hasDialog() && !detail.empty() && event.type == sf::Event::KeyPressed &&
        event.key.code == sf::Keyboard::Escape) {
        detail.clear();
        refresh();
        return;
    }
    Screen::handle(event, mouse);
}
void LessonScreen::focusedTask() {
    const auto &task = context.current.task;
    const auto &part = task.parts.front();
    const auto progress = context.school.progress(context.profile);
    title = task.title;
    setTitleWidth(520);
    setStatusArea(0, 426, 720, 3);
    status = feedback;
    note((context.settings.training ? "Тренировка" : "Занятие") + std::string(" · Задание ") +
             std::to_string(context.current.attempt.position + 1) + " из " +
             std::to_string(progress.totalTasks) + " · " + std::to_string(part.maxScore) + " б.",
         0, 0, 720);
    progressBar(progress.totalTasks ? float(progress.finishedTasks) / progress.totalTasks : 0.f, 0,
                30, 720);
    button(progress.lesson.settings.resumePolicy == ResumePolicy::ActiveTime ? "Пауза"
                                                                             : "К занятию",
           540, -64, 180, [this, progress] {
               if (!progress.lesson.closed &&
                   progress.lesson.settings.resumePolicy == ResumePolicy::ActiveTime)
                   context.school.pauseLesson();
               context.page = "home";
           });
    std::string condition =
        task.description.empty() ? part.prompt : task.description + "\n" + part.prompt;
    if (context.settings.training && !context.current.reason.empty())
        condition = context.current.reason + "\n" + condition;
    if (part.requireSteps)
        condition += "\nРешение по шагам обязательно.";
    if (part.decimalPlaces >= 0)
        condition +=
            "\nОкруглите до " + std::to_string(part.decimalPlaces) + " знаков после запятой.";
    if (part.answerTolerance > 0)
        condition += "\nДопустимая погрешность: " + std::to_string(part.answerTolerance);
    add(new TextPanel(font, condition, 720,
                      part.kind == PartKind::ShortAnswer && part.displayExpression.empty() ? 180
                                                                                           : 72),
        0, 47);
    if (part.kind != PartKind::ShortAnswer || !part.displayExpression.empty()) {
        auto formula = add(new FormulaView(headingFace(), 720, 108), 0, 128);
        formula->setFeatured(true);
        formula->setFormula(part.displayExpression.empty() ? part.expression
                                                           : part.displayExpression);
    }
    text("Ваш ответ", 0, 248, 18);
    auto answer = field(context.drafts[part.id].text, 0, 280, 480, 512);
    answer->setPlaceholder(part.kind == PartKind::ShortAnswer
                               ? "Слово, число или последовательность"
                           : part.kind == PartKind::Equation ? "Например: 1; 4"
                                                             : "Число, дробь или выражение");
    auto check = button(
        context.settings.feedback == FeedbackPolicy::AfterStep ? "Проверить ответ"
                                                               : "Сохранить ответ",
        500, 280, 220, [this] { send(true); }, !answerSubmitted);
    check->setEnabled(!answerSubmitted);
    answer->onEnter = [this] {
        if (!answerSubmitted)
            defer([this] { send(true); });
    };
    note(part.kind == PartKind::ShortAnswer ? "Введите только ответ. Порядок цифр важен; "
                                              "десятичная дробь — через точку или запятую."
         : part.kind == PartKind::Equation
             ? "Корни разделяйте через ;. Нет корней — none, все числа — all."
             : "Дробь: 1/2. Корень: sqrt(5). Степень: x^2. Десятичная дробь — через точку.",
         0, 334, 720);
    auto preview = add(new FormulaView(headingFace(), 720, 45), 0, 370);
    preview->setFormula(part.kind == PartKind::ShortAnswer ? "" : answer->getText());
    auto finish =
        button("Завершить задание →", 400, 570, 320, [this] { finishFocused(); }, answerSubmitted);
    answer->onChange = [this, answer, preview, check, finish, part] {
        context.drafts[part.id].text = answer->getText();
        context.school.saveAnswerDraft(attempt, part.id, context.drafts[part.id]);
        preview->setFormula(part.kind == PartKind::ShortAnswer ? "" : answer->getText());
        feedback.clear();
        status.clear();
        answerSubmitted = false;
        check->setEnabled(true);
        check->setPrimary(true);
        finish->setPrimary(false);
    };
    auto hint = button(context.settings.helpAllowed ? "Подсказка" : "Без подсказок", 0, 510, 160,
                       [this, part] {
                           context.school.help(attempt, part.id);
                           detail = "hint";
                           refresh();
                       });
    hint->setEnabled(context.settings.helpAllowed);
    button("Мои ответы", 175, 510, 160, [this] {
        detail = "history";
        refresh();
    });
    if (part.kind == PartKind::Equation && isLinearEquation(part.expression))
        button(part.requireSteps ? "Шаги: обязательны" : "Решить по шагам", 350, 510, 210, [this] {
            detail = "steps";
            refresh();
        });
    else if (!context.school.recommendedLectures(attempt).empty())
        button("Лекции", 350, 510, 210, [this] { lectureChoices(); });
    if (!task.resources.empty())
        button("Рисунок", 575, 510, 145, [this] { resources(0); });
    if (context.school.canSupport(attempt))
        button("Разобрать ошибку", 0, 570, 300, [this] {
            ask("Разбор завершит текущую попытку и откроет простые упражнения. Ответ сохранится. "
                "Начать?",
                [this] {
                    context.current = context.school.startSupport(attempt);
                    detail.clear();
                    refresh();
                });
        });
    focusWidget(answer);
}
void LessonScreen::finishFocused() {
    const auto &part = context.current.task.parts.front();
    std::string sent;
    bool hasAnswer = false;
    for (const auto &response : context.school.responses(attempt))
        if (response.finalAnswer && response.partId == part.id) {
            sent = response.answer.text;
            hasAnswer = true;
        }
    std::string message =
        !hasAnswer ? "Ответ ещё не отправлен. Завершить задание без ответа?"
        : sent != context.drafts[part.id].text
            ? "Изменённый ответ не отправлен. Будет учтён предыдущий ответ. Завершить?"
            : "Завершить задание? Будет учтён последний отправленный ответ.";
    ask(message, [this] {
        context.school.finish(attempt);
        context.current.attempt.finished = true;
        detail.clear();
        auto progress = context.school.progress(context.profile);
        if (!progress.lesson.settings.training && progress.finishedTasks >= progress.totalTasks)
            context.school.closeLesson();
        refresh();
    });
}
void LessonScreen::focusedDetails() {
    const auto &part = context.current.task.parts.front();
    title = detail == "steps" ? "Решение по шагам" : detail == "hint" ? "Подсказка" : "Мои ответы";
    note(context.current.task.title, 0, 0, 720);
    auto formula = add(new FormulaView(headingFace(), 720, 90), 0, 40);
    formula->setFormula(part.displayExpression.empty() ? part.expression : part.displayExpression);
    if (detail == "hint") {
        std::string hint =
            part.kind == PartKind::ShortAnswer
                ? "Перечитайте вопрос. Запишите только ответ в нужном порядке, без пояснений и "
                  "единиц измерения, если они не требуются в условии."
            : part.kind == PartKind::Equation
                ? (isLinearEquation(part.expression)
                       ? "Выполняйте одно действие с обеими частями уравнения. Перенесите числа, "
                         "затем найдите x."
                       : "Найдите дискриминант D = b² − 4ac. Затем вычислите корни и подставьте их "
                         "в исходное уравнение.")
            : part.kind == PartKind::Polynomial
                ? "Раскройте скобки и сложите коэффициенты при одинаковых степенях x."
                : "Сначала действия в скобках, затем умножение и деление. Дроби приводите к общему "
                  "знаменателю.";
        add(new TextPanel(font, hint, 720, 170), 0, 160);
        note("Подсказка учтена. Баллы за ответ сохраняются; оценка навыка учитывает помощь "
             "отдельно.",
             0, 370, 720);
    } else if (detail == "history") {
        std::string history;
        for (const auto &response : context.school.responses(attempt))
            if (response.partId == part.id)
                history += std::string(response.finalAnswer ? "Ответ: " : "Шаг: ") +
                           response.answer.text + "\n" + response.result.message + "\n\n";
        if (history.empty())
            history = "Пока нет отправленных ответов. Вернитесь к заданию и введите первый ответ.";
        add(new TextPanel(font, history, 720, 350), 0, 150);
    } else {
        text("Новое уравнение", 0, 150, 18);
        auto equation = field(stepEquation, 0, 180, 720, 512);
        equation->setPlaceholder("Например: 2*x=8");
        equation->onChange = [this, equation] { stepEquation = equation->getText(); };
        text("Действие с обеими частями", 0, 245, 18);
        auto operation =
            list({"Прибавить", "Вычесть", "Умножить", "Разделить", "Поменять стороны", "Упростить"},
                 0, 280, 340, 120);
        operation->select(stepOperation);
        operation->onChange = [this](int value) { stepOperation = value; };
        text("Число для действия", 370, 245, 18);
        auto operand = field(stepOperand, 370, 280, 160, 100);
        operand->setPlaceholder("Например: 3");
        operand->onChange = [this, operand] { stepOperand = operand->getText(); };
        auto action = [this, part] {
            const char *operations[] = {"add",    "subtract", "multiply",
                                        "divide", "swap",     "simplify"};
            AnswerPayload answer;
            answer.text = stepEquation;
            auto result = context.school.submit(attempt, part.id, answer, false,
                                                operations[stepOperation], stepOperand);
            feedback = result.message;
            answerSubmitted = false;
            refresh();
        };
        button("Проверить шаг", 370, 350, 350, action, true);
        equation->onEnter = [this, action] { defer(action); };
        setStatusArea(0, 430, 720, 3);
        status = feedback;
        focusWidget(equation);
    }
    button("← К ответу · Esc", 0, 560, 260, [this] {
        detail.clear();
        refresh();
    });
}
void LessonScreen::focusedResult() {
    title = "Задание завершено";
    auto progress = context.school.progress(context.profile);
    note(context.current.task.title, 0, 0, 720);
    surface(0, 55, 720, 150, Theme::pale);
    for (const auto &entry : context.school.history(context.profile))
        if (entry.attemptId == attempt) {
            text(scoreText(entry.result, entry.maximumScore), 24, 83, 34, 670, 2);
            note(!entry.result.visible ? entry.result.message
                 : entry.hadErrors ? "В решении были ошибки. Учтён последний отправленный ответ."
                                   : "Результат сохранён в истории.",
                 24, 160, 670);
        }
    text("Завершено " + std::to_string(progress.finishedTasks) + " из " +
             std::to_string(progress.totalTasks),
         0, 240, 24, 720, 1);
    progressBar(progress.totalTasks ? float(progress.finishedTasks) / progress.totalTasks : 0.f, 0,
                287, 720);
    note(lessonNote(progress), 0, 315, 720);
    note(
        "Баллы за задание и освоение темы — разные показатели. Навык оценивается по серии решений.",
        0, 385, 720);
    auto next = button(
        progress.lesson.closed ? "К итогам занятия →" : "Следующее задание →", 0, 470, 350,
        [this] {
            if (context.school.currentLesson(context.profile).closed) {
                context.current = NextTask();
                context.historyLesson = context.school.currentLesson(context.profile).id;
                context.historyAttempt.clear();
                context.page = "results";
            } else {
                context.current = context.school.next();
                refresh();
            }
        },
        true);
    button("Посмотреть прогресс", 370, 470, 350, [this] { context.page = "progress"; });
    if (!progress.lesson.closed) {
        if (progress.lesson.source != "practice")
            button("Продолжить позже", 0, 535, 350, [this] {
                if (context.settings.resumePolicy == ResumePolicy::ActiveTime)
                    context.school.pauseLesson();
                context.page = "home";
            });
        else
            button("Закончить тренировку", 0, 535, 350, [this] {
                ask("Завершить тренировку? Результаты сохранятся в истории.", [this] {
                    context.school.closeLesson();
                    context.current = NextTask();
                    context.historyLesson = context.school.currentLesson(context.profile).id;
                    context.historyAttempt.clear();
                    context.page = "results";
                });
            });
    }
    if (context.school.canSupport(attempt))
        button("Разобрать ошибку", 370, 535, 350, [this] {
            context.current = context.school.startSupport(attempt);
            refresh();
        });
    focusWidget(next);
}
} // namespace ui
} // namespace exam
