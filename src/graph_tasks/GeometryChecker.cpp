#include "GeometryChecker.h"
#include "math/checkers/EquationChecker.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace exam {
std::vector<double> GeometryChecker::lineCoefficients(const std::vector<Point> &points) {
    if (points.size() != 2)
        throw std::invalid_argument("Отметьте две точки прямой");
    double dx = points[1].x - points[0].x;
    if (std::abs(dx) < 1e-9)
        throw std::invalid_argument("Нужны две точки с разными x");
    double slope = (points[1].y - points[0].y) / dx;
    return {slope, points[0].y - slope * points[0].x};
}
CheckResult GeometryChecker::check(const TaskPart &part, const GraphAnswer &answer) const {
    if (part.kind != PartKind::Line && part.kind != PartKind::Parabola)
        return CheckResult(CheckStatus::Unsupported, "Тип графического задания не поддерживается");
    if (answer.points.empty() && answer.coefficients.empty())
        return CheckResult(CheckStatus::Incomplete, "Сначала постройте график");
    try {
        for (const Point &point : answer.points)
            if (!std::isfinite(point.x) || !std::isfinite(point.y))
                throw std::invalid_argument("Неверные координаты");
        Polynomial expected = polynomial(parseExpression(part.expression));
        bool line = part.kind == PartKind::Line;
        if ((line && expected.degree() > 1) || (!line && expected.degree() != 2))
            return CheckResult(CheckStatus::Unsupported, "Условие не соответствует типу графика");
        std::vector<double> values = answer.coefficients;
        if (line && !answer.points.empty())
            values = lineCoefficients(answer.points);
        if (values.size() != static_cast<std::size_t>(line ? 2 : 3))
            throw std::invalid_argument("Неверное число коэффициентов");
        if (!line && std::abs(values[0]) < 1e-9)
            throw std::invalid_argument("Коэффициент a параболы не должен быть нулём");
        for (std::size_t i = 0; i < values.size(); ++i) {
            if (!std::isfinite(values[i]))
                throw std::invalid_argument("Неверный коэффициент");
            double target = expected.coefficients[values.size() - i - 1].value();
            if (!std::isfinite(part.graphTolerance) || part.graphTolerance <= 0 ||
                part.graphTolerance > 0.1)
                throw std::invalid_argument("Неверный допуск графика");
            double tolerance = part.graphTolerance * std::max(1.0, std::abs(target));
            if (std::abs(values[i] - target) > tolerance)
                return CheckResult(CheckStatus::Incorrect, "График не соответствует функции");
        }
        return CheckResult(CheckStatus::Correct, "График построен верно");
    } catch (const MathError &error) {
        return CheckResult(CheckStatus::Unsupported, error.what(), error.position);
    } catch (const std::invalid_argument &error) {
        return CheckResult(CheckStatus::InvalidInput, error.what());
    }
}
Point Coordinates::toScreen(const Point &world) const {
    return Point(width / 2 + (world.x - center.x) * scale,
                 height / 2 - (world.y - center.y) * scale);
}
Point Coordinates::toWorld(const Point &screen) const {
    return Point(center.x + (screen.x - width / 2) / scale,
                 center.y - (screen.y - height / 2) / scale);
}
void Coordinates::zoom(double factor, const Point &anchor) {
    Point before = toWorld(anchor);
    scale = std::max(8.0, std::min(150.0, scale * factor));
    Point after = toWorld(anchor);
    center.x += before.x - after.x;
    center.y += before.y - after.y;
}
void Coordinates::pan(double dx, double dy) {
    center.x -= dx / scale;
    center.y += dy / scale;
}
double Coordinates::snap(double value, double step) {
    return step > 0 ? std::round(value / step) * step : value;
}
} // namespace exam
