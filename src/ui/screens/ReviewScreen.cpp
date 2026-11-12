#include "HomeScreen.h"
#include "ui/FileDialog.h"
#include "ui/FormulaView.h"
#include "ui/widgets/TextPanel.h"
#include "ui/ImageView.h"
#include "graphics/GraphView.h"
#include "io/Files.h"
namespace exam {
namespace ui {
namespace {
std::string verdictName(CheckStatus status) {
    if (status == CheckStatus::Correct)
        return "Верно";
    if (status == CheckStatus::Incorrect)
        return "Неверно";
    return "Нужна проверка";
}
} // namespace
void HomeScreen::reviewQueue(bool pendingOnly, bool allStudents) {
    context.school.requireTeacher();
    clear();
    title = "Проверка решений";
    reviewPending = pendingOnly;
    reviewAll = allStudents;
    button(pendingOnly ? "Без оценки · изменить" : "Все оценки · изменить", 0, 0, 300,
           [this, pendingOnly, allStudents] { reviewQueue(!pendingOnly, allStudents); });
    button(allStudents ? "Все ученики · изменить" : "Выбранный ученик · изменить", 320, 0, 300,
           [this, pendingOnly, allStudents] {
               if (allStudents && context.profile.empty())
                   throw std::runtime_error("Сначала выберите ученика в кабинете");
               reviewQueue(pendingOnly, !allStudents);
           });
    note(allStudents ? "Все профили, включая архивные" : context.profileName, 0, 60, 900);
    std::string student = allStudents ? "" : context.profile;
    auto items = context.school.reviewItems(student, pendingOnly);
    std::vector<std::string> rows;
    for (const auto &item : items) {
        std::string partName;
        const auto task = context.school.task(item.task);
        if (task.parts.size() > 1)
            for (std::size_t i = 0; i < task.parts.size(); ++i)
                if (task.parts[i].id == item.part)
                    partName = " · часть " + std::to_string(i + 1);
        rows.push_back(item.studentName + " · " + item.title + partName + " · " +
                       verdictName(item.effective));
    }
    ListBox *queue = list(rows, 0, 105, 940, 360);
    queue->setRowHeight(60);
    queue->setEmptyText("Решений для проверки нет");
    note(items.empty()
             ? "По этому фильтру нет завершённых частей для проверки."
             : "Неопределённый ответ не считается неверным. Откройте решение для проверки.",
         0, 485, 940);
    button(
        "Открыть решение", 0, 550, 280,
        [this, items, queue] {
            if (queue->selection() < 0)
                throw std::runtime_error("Выберите решение");
            reviewDetail(items[queue->selection()]);
        },
        true)
        ->setEnabled(!items.empty());
    button("Сохранить CSV", 310, 550, 280, [this, student, pendingOnly] {
        std::string path = chooseCsv();
        if (!path.empty()) {
            writeFile(path, context.school.reportCsv(student, pendingOnly));
            status = "Отчёт сохранён";
        }
    });
    button("← Кабинет", 620, 550, 280, [this] { context.page = "home"; });
}
void HomeScreen::reviewDetail(const ReviewItem &item) {
    context.school.requireTeacher();
    clear();
    title = "Ручная проверка";
    TaskVersion task = context.school.task(item.task);
    TaskPart part;
    for (const auto &candidate : task.parts)
        if (candidate.id == item.part)
            part = candidate;
    text(item.studentName + " · " + task.title, 0, 0, 21, 940, 2);
    note("История ответов и оценки сохранены. Укажите преподавателя и причину изменения.", 0, 55,
         940);
    add(new TextPanel(font, task.description + " " + part.prompt, 540, 70), 0, 90);
    FormulaView *formula = add(new FormulaView(font, 540, 70), 0, 165);
    formula->setFormula(part.displayExpression.empty() ? part.expression : part.displayExpression);
    std::vector<std::string> answers;
    GraphAnswer lastGraph;
    bool hasGraph = false;
    for (const auto &response : context.school.reviewResponses(item.attempt)) {
        if (response.partId != item.part)
            continue;
        answers.push_back(std::string(response.finalAnswer ? "Ответ: " : "Шаг: ") +
                          response.answer.text + " · " + response.operation + " " +
                          response.operand + " · " + response.result.message);
        if (part.kind == PartKind::Line || part.kind == PartKind::Parabola) {
            lastGraph = response.answer.graph;
            hasGraph = true;
        }
    }
    list(answers, 0, 245, 540, 120);
    std::vector<std::string> history;
    history.push_back("Автопроверка: " + verdictName(item.automatic));
    for (const auto &verdict : context.school.reviewHistory(item.attempt))
        if (verdict.partId == item.part)
            history.push_back(verdict.reviewer + ": " + verdictName(verdict.status) + " · " +
                              verdict.reason);
    list(history, 580, 90, 360, 155);
    if (hasGraph) {
        GraphView *graph = add(new GraphView(font, 360, 190), 580, 265);
        graph->setAnswer(part.kind, lastGraph);
        graph->setEnabled(false);
    }
    int help = 0;
    for (const auto &entry : context.school.history(item.student))
        if (entry.attemptId == item.attempt)
            help = entry.help;
    note("Подсказок за задание: " + std::to_string(help), 0, 380, 540);
    text("Преподаватель", 0, 415, 16);
    TextField *reviewer = field("", 0, 445, 230, 100);
    text("Причина решения", 250, 415, 16);
    TextField *reason = field("", 250, 445, 290, 500);
    if (!task.resources.empty())
        button("Изображения задания", 580, 475, 360, [this, item] { reviewResource(item, 0); });
    auto submit = [this, item, reviewer, reason](CheckStatus result) {
        ReviewVerdict verdict;
        verdict.requestId = newId("review");
        verdict.attemptId = item.attempt;
        verdict.partId = item.part;
        verdict.reviewer = reviewer->getText();
        verdict.reason = reason->getText();
        verdict.status = result;
        context.school.revise(verdict);
        reviewQueue(reviewPending, reviewAll);
    };
    button(
        "Верно", 0, 550, 210, [submit] { submit(CheckStatus::Correct); }, true);
    button("Неверно", 230, 550, 210, [submit] { submit(CheckStatus::Incorrect); });
    button("Оставить без оценки", 460, 550, 250, [submit] { submit(CheckStatus::Unsupported); });
    button("← Список", 730, 550, 210, [this] { reviewQueue(reviewPending, reviewAll); });
}
void HomeScreen::reviewResource(const ReviewItem &item, int index) {
    context.school.requireTeacher();
    auto task = context.school.task(item.task);
    clear();
    title = "Ресурс задания";
    add(new ImageView(font, resourcePath("data", task.resources.at(index))), 0, 0);
    button("← К решению", 0, 550, 300, [this, item] { reviewDetail(item); });
    if (index + 1 < static_cast<int>(task.resources.size()))
        button("Следующее изображение", 600, 550, 340,
               [this, item, index] { reviewResource(item, index + 1); });
}
} // namespace ui
} // namespace exam
