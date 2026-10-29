#ifndef SMARTEXAM_GEOMETRY_CHECKER_H
#define SMARTEXAM_GEOMETRY_CHECKER_H
#include "core/Services.h"
namespace exam {
struct Interval {
    double left, right;
    bool includeLeft, includeRight;
    Interval() : left(0), right(0), includeLeft(false), includeRight(false) {}
};
class GeometryChecker : public GraphChecker {
  public:
    CheckResult check(const TaskPart &part, const GraphAnswer &answer) const override;
    static std::vector<double> lineCoefficients(const std::vector<Point> &points);
};
class Coordinates {
  public:
    Point center;
    double scale, width, height;
    Coordinates(double w = 500, double h = 350) : scale(35), width(w), height(h) {}
    Point toScreen(const Point &world) const;
    Point toWorld(const Point &screen) const;
    void zoom(double factor, const Point &screenAnchor);
    void pan(double dx, double dy);
    static double snap(double value, double step);
};
} // namespace exam
#endif
