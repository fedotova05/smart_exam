#include "AnswerPolicy.h"
#include "math/checkers/EquationChecker.h"
#include <cmath>
#include <set>
#include <sstream>
#include <windows.h>
#ifndef LOCALE_INVARIANT
#define LOCALE_INVARIANT 0x007f
#endif
namespace exam {
namespace {
std::wstring shortAnswer(const std::string &text) {
    if (text.size() > 512 || text.find('\0') != std::string::npos)
        throw MathError("Ответ: не более 512 байтов, без служебных символов");
    if (text.empty())
        return L"";
    int size =
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), 0, 0);
    if (!size)
        throw MathError("Неверная кодировка ответа");
    std::wstring result(size, L' ');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), &result[0],
                        size);
    const auto first = result.find_first_not_of(L" \t\r\n\u00a0");
    if (first == std::wstring::npos)
        return L"";
    result = result.substr(first, result.find_last_not_of(L" \t\r\n\u00a0") - first + 1);
    for (wchar_t c : result)
        if (c < 32 || c == 127)
            throw MathError("Введите ответ одной строкой");
    std::wstring lower(result.size(), L' ');
    if (!LCMapStringW(LOCALE_INVARIANT, LCMAP_LOWERCASE, result.data(), int(result.size()),
                      &lower[0], int(lower.size())))
        throw MathError("Не удалось прочитать ответ");
    bool decimal = true, digit = false;
    int separators = 0;
    for (std::size_t i = 0; i < lower.size(); ++i) {
        wchar_t c = lower[i];
        if (c >= L'0' && c <= L'9')
            digit = true;
        else if (c == L'.' || c == L',')
            ++separators;
        else if (i != 0 || (c != L'-' && c != L'+'))
            decimal = false;
    }
    if (decimal && digit && separators == 1)
        for (auto &c : lower)
            if (c == L',')
                c = L'.';
    return lower;
}
double numeric(const Radical &value) {
    return double(value.a.numerator()) / value.a.denominator() +
           double(value.b.numerator()) / value.b.denominator() * std::sqrt(double(value.d));
}
std::set<int> selected(const std::string &text, int count) {
    std::set<int> result;
    std::istringstream stream(text);
    std::string token;
    if (text.empty() || text.back() == ';')
        throw MathError("Укажите номера вариантов через ;");
    while (std::getline(stream, token, ';')) {
        auto start = token.find_first_not_of(" \t");
        auto end = token.find_last_not_of(" \t");
        if (start == std::string::npos)
            throw MathError("Пустой номер варианта");
        token = token.substr(start, end - start + 1);
        if (token.size() > 2 || token.find_first_not_of("0123456789") != std::string::npos)
            throw MathError("Ожидался номер варианта");
        int id = std::stoi(token);
        if (id < 1 || id > count || !result.insert(id).second)
            throw MathError("Неверный или повторный номер варианта");
    }
    return result;
}
} // namespace
std::string answerPolicyError(const TaskPart &part) {
    if (part.expression.size() > 512 || part.decimalPlaces < -1 || part.decimalPlaces > 6 ||
        !std::isfinite(part.answerTolerance) || part.answerTolerance < 0 ||
        part.answerTolerance > 1)
        return "Формула до 512 байтов; округление от 0 до 6 знаков; допуск от 0 до 1";
    if ((part.decimalPlaces >= 0 || part.answerTolerance > 0) && part.kind != PartKind::Number &&
        part.kind != PartKind::Expression)
        return "Округление и числовой допуск доступны только для числового ответа";
    if (part.requireSteps &&
        (part.kind != PartKind::Equation || !isLinearEquation(part.expression)))
        return "Обязательные шаги поддерживаются только для линейного уравнения";
    try {
        switch (part.kind) {
        case PartKind::ShortAnswer:
            if (shortAnswer(part.correctAnswer).empty())
                return "Введите правильный ответ: слово, число или последовательность";
            break;
        case PartKind::Expression:
        case PartKind::Number:
            evaluateExact(parseExpression(part.expression));
            break;
        case PartKind::Equation:
            solveEquation(part.expression);
            break;
        case PartKind::Polynomial:
            polynomial(parseExpression(part.expression));
            break;
        case PartKind::Choice: {
            if (part.choices.size() < 2 || part.choices.size() > 12 || part.correctChoices.empty())
                return "Нужно от 2 до 12 вариантов и хотя бы один правильный";
            std::set<int> ids;
            for (const auto &text : part.choices)
                if (text.empty() || text.size() > 256)
                    return "Текст варианта: от 1 до 256 байтов";
            for (int id : part.correctChoices)
                if (id < 1 || id > int(part.choices.size()) || !ids.insert(id).second)
                    return "Неверный или повторный правильный вариант";
            break;
        }
        case PartKind::Line:
        case PartKind::Parabola: {
            int degree = polynomial(parseExpression(part.expression)).degree();
            if ((part.kind == PartKind::Line && degree > 1) ||
                (part.kind == PartKind::Parabola && degree != 2))
                return "Формула не соответствует типу графика";
            break;
        }
        default:
            return "Неподдерживаемый формат ответа";
        }
    } catch (const std::exception &error) {
        return error.what();
    }
    return "";
}
CheckResult checkStructuredAnswer(const TaskPart &part, const AnswerPayload &answer) {
    if (answer.text.find_first_not_of(" \t\r\n") == std::string::npos)
        return CheckResult(CheckStatus::Incomplete, "Введите ответ");
    if (answer.text.size() > 512)
        return CheckResult(CheckStatus::Unsupported, "Ответ длиннее 512 байтов");
    try {
        bool correct = false;
        if (part.kind == PartKind::ShortAnswer) {
            auto actual = shortAnswer(answer.text);
            if (actual.empty())
                return CheckResult(CheckStatus::Incomplete, "Введите ответ");
            correct = actual == shortAnswer(part.correctAnswer);
        } else if (part.kind == PartKind::Choice)
            correct = selected(answer.text, int(part.choices.size())) ==
                      std::set<int>(part.correctChoices.begin(), part.correctChoices.end());
        else if (part.kind == PartKind::Polynomial)
            correct = polynomial(parseExpression(part.expression)) ==
                      polynomial(parseExpression(answer.text));
        else {
            Radical expected = evaluateExact(parseExpression(part.expression));
            Radical actual = evaluateExact(parseExpression(answer.text));
            if (part.decimalPlaces < 0 && part.answerTolerance == 0)
                correct = expected == actual;
            else {
                double target = numeric(expected);
                if (part.decimalPlaces >= 0) {
                    double scale = std::pow(10.0, part.decimalPlaces);
                    target = std::round(target * scale) / scale;
                }
                correct = std::abs(numeric(actual) - target) <= part.answerTolerance + 1e-12;
            }
        }
        return CheckResult(correct ? CheckStatus::Correct : CheckStatus::Incorrect,
                           correct ? "Верно" : "Ответ не совпадает с решением");
    } catch (const MathError &error) {
        return CheckResult(error.unsupported ? CheckStatus::Unsupported : CheckStatus::InvalidInput,
                           error.what(), error.position);
    }
}
bool solvedLinearStep(const std::string &text) {
    try {
        Equation equation = parseEquation(text);
        Polynomial left = polynomial(equation.left), right = polynomial(equation.right), variable;
        variable.coefficients[1] = Rational(1);
        return (left == variable && right.degree() == 0) ||
               (right == variable && left.degree() == 0);
    } catch (const MathError &) {
        return false;
    }
}
} // namespace exam
