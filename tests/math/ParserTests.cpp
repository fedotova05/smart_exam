#include "../Test.h"
#include "math/parser/Parser.h"
using namespace exam;
void testMath() {
    REQUIRE(Rational(1, 3) + Rational(1, 6) == Rational(1, 2));
    REQUIRE(Rational(4, -6) == Rational(-2, 3));
    REQUIRE(Rational(1000000000, 3) * Rational(3, 1000000000) == Rational(1));
    REQUIRE(evaluateNumber(parseExpression("1,25 + .75")) == Rational(2));
    REQUIRE(evaluateNumber(parseExpression("-2^2")) == Rational(-4));
    REQUIRE(evaluateNumber(parseExpression("(-2)^2")) == Rational(4));
    REQUIRE(evaluateNumber(parseExpression("2^2^3")) == Rational(256));
    REQUIRE(parseExpression("2x(x+1)")->kind == NodeKind::Multiply);
    REQUIRE(parseExpression("-x^2")->kind == NodeKind::Negate);
    REQUIRE(parseExpression("(-x)^2")->kind == NodeKind::Power);
    bool rejected = false;
    try {
        evaluateNumber(parseExpression("1/0"));
    } catch (const MathError &e) {
        rejected = !e.unsupported;
    }
    REQUIRE(rejected);
    rejected = false;
    try {
        Rational(1000000000) * Rational(1000000000);
    } catch (const MathError &e) {
        rejected = e.unsupported;
    }
    REQUIRE(rejected);
    rejected = false;
    try {
        parseExpression("2+)");
    } catch (const MathError &e) {
        rejected = e.position == 2;
    }
    REQUIRE(rejected);
    rejected = false;
    try {
        parseExpression(std::string(513, '1'));
    } catch (const MathError &e) {
        rejected = e.unsupported;
    }
    REQUIRE(rejected);
    rejected = false;
    try {
        parseExpression(std::string(70, '-') + "1");
    } catch (const MathError &e) {
        rejected = e.unsupported;
    }
    REQUIRE(rejected);
    for (const std::string &input : {"", "(", "sqrt(", "1..2", "2 3", "x=1", "2@3", "1,"}) {
        rejected = false;
        try {
            parseExpression(input);
        } catch (const MathError &) {
            rejected = true;
        }
        REQUIRE(rejected);
    }
}
