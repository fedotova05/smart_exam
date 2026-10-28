#include "../Test.h"
#include "math/checkers/EquationChecker.h"
using namespace exam;
void testChecker() {
    REQUIRE(evaluateExact(parseExpression("sqrt(8)"))==evaluateExact(parseExpression("2*sqrt(2)")));
    REQUIRE(evaluateExact(parseExpression("sqrt(1/2)"))==evaluateExact(parseExpression("sqrt(2)/2")));
    REQUIRE(solveEquation("x=x").kind==RootKind::AllReal);
    REQUIRE(solveEquation("x=x+1").values.empty());
    REQUIRE(solveEquation("x^2+1=0").values.empty());
    REQUIRE(solveEquation("x^2-2*x+1=0").values.size()==1);
    REQUIRE(solveEquation("x^2=2").values.size()==2);
    UnavailableGraphChecker graph; EquationChecker checker(graph);
    TaskPart task; task.expression="x^2=2"; AnswerPayload answer;
    answer.text="-sqrt(2);sqrt(2)"; REQUIRE(checker.checkAnswer(task,answer).status==CheckStatus::Correct);
    answer.text="sqrt(2)"; REQUIRE(checker.checkAnswer(task,answer).status==CheckStatus::Incorrect);
    answer.text=""; REQUIRE(checker.checkAnswer(task,answer).status==CheckStatus::Incomplete);
    REQUIRE(checker.checkStep("2x+2=4","x+1=2").status==CheckStatus::Correct);
    REQUIRE(checker.checkStep("2x+2=4","2x=2","subtract","2").status==CheckStatus::Correct);
    REQUIRE(checker.checkStep("2x+2=4","x+1=2","subtract","2").status==CheckStatus::Incorrect);
    REQUIRE(checker.checkStep("2x+2=4","x+1=2","divide","2").status==CheckStatus::Correct);
    REQUIRE(checker.checkStep("x^2=1","x=1").status==CheckStatus::Incorrect);
    REQUIRE(checker.checkStep("x=1","0=0","multiply","0").status==CheckStatus::Incorrect);
    REQUIRE(checker.checkStep("x/x=1","1=1").status==CheckStatus::Unsupported);
    REQUIRE(checker.checkStep("x=1","x=1/0").status==CheckStatus::InvalidInput);
    for(int first=-4;first<=4;++first) for(int second=-4;second<=4;++second) {
        Polynomial p; p.coefficients[2]=Rational(1); p.coefficients[1]=Rational(-first-second); p.coefficients[0]=Rational(first*second);
        Roots roots=solve(p); REQUIRE(roots.values.size()==static_cast<std::size_t>(first==second?1:2));
        for(const Radical& r:roots.values) { REQUIRE(r.b.zero()); REQUIRE(p.at(r.a).zero()); }
    }
}
