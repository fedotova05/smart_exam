#ifndef SMARTEXAM_EQUATION_CHECKER_H
#define SMARTEXAM_EQUATION_CHECKER_H
#include "core/Services.h"
#include "math/parser/Parser.h"
#include <array>
namespace exam {
struct Radical {
    Rational a, b;
    int d;
    Radical(Rational rational = Rational(), Rational coefficient = Rational(), int radicand = 0);
    std::string text() const;
};
bool operator==(const Radical &left, const Radical &right);
Radical operator+(const Radical &left, const Radical &right);
Radical operator-(const Radical &value);
Radical operator*(const Radical &left, const Radical &right);
Radical squareRoot(const Rational &value);
Radical evaluateExact(const Expr &expression);
struct Polynomial {
    std::array<Rational, 3> coefficients;
    explicit Polynomial(Rational constant = Rational());
    int degree() const;
    Rational at(const Rational &x) const;
};
Polynomial polynomial(const Expr &expression);
Polynomial operator+(const Polynomial &left, const Polynomial &right);
Polynomial operator-(const Polynomial &left, const Polynomial &right);
Polynomial operator*(const Polynomial &left, const Polynomial &right);
bool operator==(const Polynomial &left, const Polynomial &right);
enum class RootKind { Finite, AllReal };
struct Roots {
    RootKind kind;
    std::vector<Radical> values;
    Roots() : kind(RootKind::Finite) {}
};
Roots solve(const Polynomial &value);
Roots solveEquation(const std::string &text);
bool sameRoots(const Roots &left, const Roots &right);
class EquationChecker : public Checker {
    const GraphChecker &graphChecker;

  public:
    explicit EquationChecker(const GraphChecker &graph) : graphChecker(graph) {}
    CheckResult checkAnswer(const TaskPart &, const AnswerPayload &) const override;
    CheckResult checkStep(const std::string &before, const std::string &after,
                          const std::string &operation = "",
                          const std::string &operand = "") const override;
};
} // namespace exam
#endif
