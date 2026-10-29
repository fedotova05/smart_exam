#include "AppContext.h"
#include <sstream>
#include <locale>
#include <cmath>
namespace exam {
namespace ui {
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
} // namespace ui
} // namespace exam
