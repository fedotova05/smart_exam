#include "../Test.h"
#include "generation/Generator.h"
#include "math/checkers/EquationChecker.h"
using namespace exam;
void testGeneration() {
    Generator generator;
    for(Family family:{Family::Expression,Family::Linear,Family::Quadratic,Family::Line,Family::Parabola}) {
        for(unsigned seed=0;seed<50;++seed) {
            GenerationRequest request; request.id="test"; request.seed=seed; request.family=family;
            TaskVersion task=generator.generateTask(request);
            REQUIRE(task.parts.front().expression==generator.generateTask(request).parts.front().expression);
            if(family==Family::Expression) evaluateExact(parseExpression(task.parts[0].expression));
            else {
                Equation equation=parseEquation(task.parts[0].expression);
                Polynomial p=polynomial(equation.left)-polynomial(equation.right);
                Roots roots=solve(p); REQUIRE(!roots.values.empty());
                for(const Radical& root:roots.values) {
                    Radical value=Radical(p.coefficients[2])*root*root+Radical(p.coefficients[1])*root+Radical(p.coefficients[0]);
                    REQUIRE(value==Radical());
                }
                if(task.parts.size()==2) REQUIRE(polynomial(parseExpression(task.parts[1].expression))==p);
            }
        }
    }
    GenerationRequest request; request.id="generated"; request.maxAttempts=1;
    request.previousExpressions.insert(generator.generateTask(request).parts[0].expression);
    bool rejected=false; try { generator.generateTask(request); } catch(const std::runtime_error&) { rejected=true; } REQUIRE(rejected);
    Store store(":memory:"); store.saveTopic("algebra","Алгебра"); store.saveSkill("linear","algebra","Линейное уравнение");
    request.previousExpressions.clear();
    TaskVersion saved=generator.generateAndSave(store,request); store.publish(saved.id);
    REQUIRE(store.task(saved.id).generator=="math-1/1");
    REQUIRE(store.task(saved.id).parameters==saved.parameters);
    REQUIRE(store.task(saved.id).seed==request.seed);
}
