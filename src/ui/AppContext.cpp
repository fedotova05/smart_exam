#include "AppContext.h"
#include <sstream>
#include <locale>
#include <cmath>
namespace exam {
namespace ui {
bool canOpen(UserRole role, const std::string &page) {
    if (page == "welcome")
        return true;
    if (role == UserRole::Guest)
        return false;
    if (page == "home" || page == "results" || page == "knowledge" || page == "setup" ||
        page == "progress" || page == "practice" || page == "training-demo")
        return true;
    if (page == "lesson")
        return role == UserRole::Student;
    return role == UserRole::Teacher &&
           (page == "bank" || page == "packages" || page == "review" || page == "profiles");
}
std::vector<std::string> splitCodes(const std::string &text) {
    std::string separated = text;
    for (char &c : separated)
        if (c == ',' || c == ';')
            c = ' ';
    std::istringstream input(separated);
    std::vector<std::string> codes;
    std::string code;
    while (input >> code)
        codes.push_back(code);
    return codes;
}
std::string joinCodes(const std::vector<std::string> &codes) {
    std::string result;
    for (const auto &code : codes) {
        if (!result.empty())
            result += ", ";
        result += code;
    }
    return result;
}
std::string kindName(PartKind kind) {
    switch (kind) {
    case PartKind::Expression:
        return "Значение";
    case PartKind::ShortAnswer:
        return "Слово или число";
    case PartKind::Number:
        return "Число";
    case PartKind::Choice:
        return "Выбор вариантов";
    case PartKind::Polynomial:
        return "Выражение с x";
    case PartKind::Equation:
        return "Уравнение";
    case PartKind::Line:
        return "Прямая";
    case PartKind::Parabola:
        return "Парабола";
    }
    return "";
}
std::string resultText(const AttemptResult &result) {
    if (!result.visible)
        return result.message;
    return std::to_string(result.score) + " балл. " +
           (result.provisional ? "Предварительная оценка. " : "") + result.message;
}
double readNumber(const std::string &text) {
    std::istringstream input(text);
    input.imbue(std::locale::classic());
    double number;
    char extra;
    if (!(input >> number) || (input >> extra) || !std::isfinite(number) ||
        std::abs(number) > 10000)
        throw std::invalid_argument("Введите число от −10000 до 10000, дробную часть через точку");
    return number;
}
std::string scoreText(const AttemptResult &result, int maximum) {
    if (!result.finished)
        return "В работе";
    if (!result.visible)
        return "Оценка пока скрыта";
    return std::to_string(result.score) + " / " + std::to_string(maximum) +
           (result.provisional ? " · не все части проверены" : " балл.");
}
std::string skillEstimate(const SkillState &skill) {
    if (!skill.known || skill.observations == 0)
        return "Нет данных";
    int percent = static_cast<int>(std::round(skill.mastery * 100));
    if (percent >= 100)
        return "> 99%";
    if (percent <= 0)
        return "< 1%";
    return "≈ " + std::to_string(percent) + "%";
}
std::string skillEvidence(const SkillState &skill) {
    if (!skill.known || skill.observations == 0)
        return "Решений ещё нет. Это не нулевой результат.";
    if (skill.observations == 1)
        return "Одно наблюдение. Для вывода об освоении нужно больше практики.";
    return "Наблюдений: " + std::to_string(skill.observations) +
           ". Оценка по истории, а не гарантия знания темы.";
}
std::string lessonScore(const LessonProgress &progress) {
    if (progress.lesson.id.empty())
        return "Нет занятия";
    if (progress.hidden)
        return "Пока скрыто";
    return std::to_string(progress.score) + " / " + std::to_string(progress.maximumScore);
}
std::string lessonNote(const LessonProgress &progress) {
    if (progress.lesson.id.empty())
        return "Начните занятие: здесь появятся баллы и ход работы.";
    if (progress.hidden)
        return "Оценка появится после завершения занятия.";
    if (progress.provisional)
        return "Предварительно: есть неотвеченные или непроверенные части.";
    if (progress.lesson.settings.training && progress.lesson.closed)
        return "Тренировка завершена. Результаты разбора сохранены в истории и оценках навыков.";
    if (progress.lesson.closed && progress.finishedTasks < progress.totalTasks)
        return "Занятие закончено раньше плана. Остались невыполненные задания.";
    if (progress.finishedTasks < progress.totalTasks)
        return "Это текущие баллы. Остальные задания ещё впереди.";
    return "Занятие выполнено. Его результат не равен освоению всей темы.";
}
} // namespace ui
} // namespace exam
