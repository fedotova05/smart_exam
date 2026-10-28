#include "Generator.h"
#include "math/checkers/EquationChecker.h"
#include <random>
#include <stdexcept>
namespace exam {
namespace {
int integer(std::mt19937& random,int low,int high) { return low+static_cast<int>(random()%static_cast<unsigned>(high-low+1)); }
std::string polynomialText(int a,int b,int c) {
    return std::to_string(a)+"*x^2+("+std::to_string(b)+")*x+("+std::to_string(c)+")";
}
TaskPart makePart(const std::string& id,PartKind kind,const std::string& prompt,const std::string& expression,const std::string& skill) {
    TaskPart part; part.id=id; part.kind=kind; part.prompt=prompt; part.expression=expression; part.skills.push_back(skill); return part;
}
}
TaskVersion Generator::generateTask(const GenerationRequest& request) const {
    if(request.id.empty() || request.maxAttempts<1 || request.maxAttempts>1000) throw std::invalid_argument("Invalid generation request");
    for(int attempt=0;attempt<request.maxAttempts;++attempt) {
        TaskVersion task; task.id=request.id; task.seed=request.seed+static_cast<unsigned>(attempt);
        task.generator="math-1/"+std::to_string(static_cast<int>(request.family));
        std::mt19937 random(task.seed);
        int a=integer(random,1,4), first=integer(random,-9,9),second=integer(random,-9,9);
        if(request.family==Family::Expression) {
            int denominator=integer(random,2,9);
            std::string expression="("+std::to_string(first)+")/"+std::to_string(a)+"+("+std::to_string(second)+")/"+std::to_string(denominator);
            task.title="Действия с дробями"; task.difficulty=850;
            task.parts.push_back(makePart("answer",PartKind::Expression,"Вычислите точно",expression,"fractions"));
            task.parameters="n1="+std::to_string(first)+";d1="+std::to_string(a)+";n2="+std::to_string(second)+";d2="+std::to_string(denominator);
        } else {
            int b,c;
            bool linear=request.family==Family::Linear || request.family==Family::Line;
            if(linear) { b=a; c=first; a=0; task.difficulty=1000; }
            else {
                b=-a*(first+second); c=a*first*second; task.difficulty=1150;
                if(task.seed%3==0) { a=1; b=0; c=-integer(random,2,15); }
            }
            std::string expression=polynomialText(a,b,c);
            task.title=linear?"Линейная функция":"Квадратичная функция";
            task.parameters="a="+std::to_string(a)+";b="+std::to_string(b)+";c="+std::to_string(c);
            task.parts.push_back(makePart("roots",PartKind::Equation,"Найдите нули функции",expression+"=0",linear?"linear":"quadratic"));
            if(request.family==Family::Line || request.family==Family::Parabola) {
                task.parts.push_back(makePart("graph",linear?PartKind::Line:PartKind::Parabola,"Постройте график той же функции",expression,linear?"line_graph":"parabola_graph"));
            }
        }
        if(request.previousExpressions.count(task.parts.front().expression)==0) return task;
    }
    throw std::runtime_error("Не удалось получить неповторяющееся задание за заданное число попыток");
}
TaskVersion Generator::generateAndSave(Store& store,const GenerationRequest& request) const {
    TaskVersion task=generateTask(request);
    store.saveDraft(task);
    return store.task(task.id);
}
}
