#include "../Test.h"
#include "math/checkers/EquationChecker.h"
#include "graph_tasks/GeometryChecker.h"
#include <chrono>
#include <cmath>
using namespace exam;
void testRegression() {
    GeometryChecker geometry;
    EquationChecker checker(geometry);
    auto started = std::chrono::steady_clock::now();
    int cases = 0;
    for (int a = -8; a <= 8; ++a)
        for (int b = -8; b <= 8; ++b) {
            TaskPart part;
            part.kind = PartKind::Equation;
            part.expression =
                "x^2-(" + std::to_string(a + b) + ")*x+(" + std::to_string(a * b) + ")=0";
            AnswerPayload value;
            value.text = std::to_string(a) + (a == b ? "" : ";" + std::to_string(b));
            REQUIRE(checker.checkAnswer(part, value).status == CheckStatus::Correct);
            value.text = std::to_string(a + 20);
            REQUIRE(checker.checkAnswer(part, value).status == CheckStatus::Incorrect);
            ++cases;
        }
    for (int a = -12; a <= 12; ++a)
        for (int b = 1; b <= 12; ++b) {
            TaskPart part;
            part.kind = PartKind::Expression;
            part.expression = "(" + std::to_string(a) + "/" + std::to_string(b) + ")+(" +
                              std::to_string(b - a) + "/" + std::to_string(b) + ")";
            AnswerPayload value;
            value.text = "1";
            REQUIRE(checker.checkAnswer(part, value).status == CheckStatus::Correct);
            ++cases;
        }
    for (auto expression : {"1/0", "sqrt(-1)", "999999999^8", "sqrt(", "x^3=1"}) {
        TaskPart part;
        part.kind = PartKind::Expression;
        part.expression = expression;
        AnswerPayload value;
        value.text = "0";
        REQUIRE(checker.checkAnswer(part, value).status != CheckStatus::Correct);
    }
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now() - started)
                       .count();
    REQUIRE(elapsed < 5000);
    std::cout << "Independent arithmetic/equation cases: " << cases << ", " << elapsed << " ms\n";
}
