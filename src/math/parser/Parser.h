#ifndef SMARTEXAM_PARSER_H
#define SMARTEXAM_PARSER_H
#include "math/numbers/Rational.h"
#include <memory>
#include <vector>
namespace exam {
enum class NodeKind {
    Number,
    Variable,
    Add,
    Subtract,
    Multiply,
    Divide,
    Negate,
    Power,
    SquareRoot
};
struct Expression {
    NodeKind kind;
    Rational number;
    int position;
    std::shared_ptr<Expression> left, right;
    Expression(NodeKind type, int at) : kind(type), position(at) {}
};
typedef std::shared_ptr<Expression> Expr;
struct Equation {
    Expr left, right;
};
struct Token {
    char kind;
    std::string text;
    int position;
};
std::vector<Token> tokenize(const std::string &text);
Expr parseExpression(const std::string &text);
Equation parseEquation(const std::string &text);
Rational evaluateNumber(const Expr &expression);
} // namespace exam
#endif
