#include "EquationChecker.h"
#include <algorithm>
#include <sstream>
namespace exam {
Radical::Radical(Rational rational,Rational coefficient,int radicand) : a(rational),b(coefficient),d(radicand) {
    if(d<0) throw MathError("Корень отрицательного числа");
    if(d>1000000) throw MathError("Подкоренное число превышает 1000000",true);
    if(d==0 || b.zero()) { b=Rational(); d=0; return; }
    for(int factor=2;factor*factor<=d;++factor) {
        while(d%(factor*factor)==0) { d/=factor*factor; b=b*Rational(factor); }
    }
    if(d==1) { a=a+b; b=Rational(); d=0; }
}
bool operator==(const Radical& left,const Radical& right) { return left.a==right.a && left.b==right.b && left.d==right.d; }
Radical operator+(const Radical& left,const Radical& right) {
    if(left.d!=right.d && !left.b.zero() && !right.b.zero()) throw MathError("Сумма разных радикалов не поддерживается",true);
    return Radical(left.a+right.a,left.b+right.b,left.b.zero()?right.d:left.d);
}
Radical operator-(const Radical& value) { return Radical(-value.a,-value.b,value.d); }
Radical operator*(const Radical& left,const Radical& right) {
    if(left.b.zero()) return Radical(left.a*right.a,left.a*right.b,right.d);
    if(right.b.zero()) return right*left;
    if(left.d!=right.d) throw MathError("Произведение разных радикалов не поддерживается",true);
    return Radical(left.a*right.a+left.b*right.b*Rational(left.d),left.a*right.b+left.b*right.a,left.d);
}
std::string Radical::text() const {
    if(b.zero()) return a.text();
    return "("+a.text()+"+("+b.text()+")*sqrt("+std::to_string(d)+"))";
}
Radical squareRoot(const Rational& value) {
    if(value<Rational()) throw MathError("Корень отрицательного числа");
    std::int64_t radicand=value.numerator()*value.denominator();
    if(radicand>1000000) throw MathError("Подкоренное число превышает 1000000",true);
    return Radical(Rational(),Rational(1,value.denominator()),static_cast<int>(radicand));
}
Radical evaluateExact(const Expr& e) {
    if(e->kind==NodeKind::Number) return Radical(e->number);
    if(e->kind==NodeKind::Variable) throw MathError("В ответе ожидается число",false,e->position);
    if(e->kind==NodeKind::SquareRoot) return squareRoot(evaluateNumber(e->left));
    Radical left=evaluateExact(e->left);
    if(e->kind==NodeKind::Negate) return -left;
    Radical right=evaluateExact(e->right);
    switch(e->kind) {
    case NodeKind::Add: return left+right;
    case NodeKind::Subtract: return left+(-right);
    case NodeKind::Multiply: return left*right;
    case NodeKind::Divide:
        if(!right.b.zero()) throw MathError("Деление на радикал не поддерживается",true,e->position);
        return Radical(left.a/right.a,left.b/right.a,left.d);
    case NodeKind::Power: {
        if(!right.b.zero() || right.a.denominator()!=1 || right.a.numerator()<0 || right.a.numerator()>8) throw MathError("Неподдерживаемая степень",true,e->position);
        if(left==Radical() && right.a.zero()) throw MathError("Неопределённость 0^0");
        Radical result(Rational(1)); for(int i=0;i<right.a.numerator();++i) result=result*left; return result;
    }
    default: throw MathError("Неподдерживаемая операция",true,e->position);
    }
}
Polynomial::Polynomial(Rational constant) { coefficients[0]=constant; }
int Polynomial::degree() const { for(int i=2;i>=0;--i) if(!coefficients[i].zero()) return i; return 0; }
Rational Polynomial::at(const Rational& x) const { return (coefficients[2]*x+coefficients[1])*x+coefficients[0]; }
Polynomial operator+(const Polynomial& a,const Polynomial& b) {
    Polynomial result; for(int i=0;i<3;++i) result.coefficients[i]=a.coefficients[i]+b.coefficients[i]; return result;
}
Polynomial operator-(const Polynomial& a,const Polynomial& b) {
    Polynomial result; for(int i=0;i<3;++i) result.coefficients[i]=a.coefficients[i]-b.coefficients[i]; return result;
}
Polynomial operator*(const Polynomial& a,const Polynomial& b) {
    if(a.degree()+b.degree()>2) throw MathError("Поддерживаются многочлены до второй степени",true);
    Polynomial result;
    for(int i=0;i<3;++i) for(int j=0;j+i<3;++j) result.coefficients[i+j]=result.coefficients[i+j]+a.coefficients[i]*b.coefficients[j];
    return result;
}
bool operator==(const Polynomial& a,const Polynomial& b) { return a.coefficients==b.coefficients; }
Polynomial polynomial(const Expr& e) {
    if(e->kind==NodeKind::Number) return Polynomial(e->number);
    if(e->kind==NodeKind::Variable) { Polynomial value; value.coefficients[1]=Rational(1); return value; }
    if(e->kind==NodeKind::SquareRoot) {
        Radical value=evaluateExact(e);
        if(!value.b.zero()) throw MathError("Коэффициенты уравнения должны быть рациональными",true,e->position);
        return Polynomial(value.a);
    }
    Polynomial left=polynomial(e->left);
    if(e->kind==NodeKind::Negate) return Polynomial()-left;
    Polynomial right=polynomial(e->right);
    switch(e->kind) {
    case NodeKind::Add: return left+right;
    case NodeKind::Subtract: return left-right;
    case NodeKind::Multiply: return left*right;
    case NodeKind::Divide:
        if(right.degree()!=0) throw MathError("Деление на выражение с x не поддерживается",true,e->position);
        return left*Polynomial(Rational(1)/right.coefficients[0]);
    case NodeKind::Power: {
        Rational exponent=right.coefficients[0];
        if(right.degree()!=0 || exponent.denominator()!=1 || exponent.numerator()<0 || exponent.numerator()>8) throw MathError("Неподдерживаемая степень",true,e->position);
        if(left==Polynomial() && exponent.zero()) throw MathError("Неопределённость 0^0");
        Polynomial result(Rational(1)); for(int i=0;i<exponent.numerator();++i) result=result*left; return result;
    }
    default: throw MathError("Неподдерживаемая операция",true,e->position);
    }
}
Roots solve(const Polynomial& p) {
    Roots result;
    const Rational& c=p.coefficients[0]; const Rational& b=p.coefficients[1]; const Rational& a=p.coefficients[2];
    if(a.zero()) {
        if(!b.zero()) result.values.push_back(Radical(-c/b));
        else if(c.zero()) result.kind=RootKind::AllReal;
        return result;
    }
    Rational discriminant=b*b-Rational(4)*a*c;
    if(discriminant<Rational()) return result;
    Radical root=squareRoot(discriminant);
    Rational denominator=Rational(2)*a;
    result.values.push_back(Radical(-b/denominator,root.b/denominator,root.d)+Radical(root.a/denominator));
    if(!discriminant.zero()) result.values.push_back(Radical(-b/denominator,-root.b/denominator,root.d)+Radical(-root.a/denominator));
    return result;
}
Roots solveEquation(const std::string& text) { Equation e=parseEquation(text); return solve(polynomial(e.left)-polynomial(e.right)); }
bool sameRoots(const Roots& a,const Roots& b) {
    if(a.kind!=b.kind || a.values.size()!=b.values.size()) return false;
    for(const Radical& value:a.values) if(std::find(b.values.begin(),b.values.end(),value)==b.values.end()) return false;
    return true;
}
namespace {
CheckResult errorResult(const MathError& error) { return CheckResult(error.unsupported?CheckStatus::Unsupported:CheckStatus::InvalidInput,error.what(),error.position); }
std::string trim(const std::string& text) {
    std::size_t start=text.find_first_not_of(" \t\r\n");
    if(start==std::string::npos) return "";
    return text.substr(start,text.find_last_not_of(" \t\r\n")-start+1);
}
Roots readRoots(std::string text) {
    text=trim(text); Roots result;
    if(text=="all" || text=="R" || text=="все") { result.kind=RootKind::AllReal; return result; }
    if(text=="none" || text=="{}" || text=="нет корней") return result;
    if(text.size()>1 && text.front()=='{' && text.back()=='}') text=text.substr(1,text.size()-2);
    if(text.empty()) throw MathError("Пустой список корней");
    std::size_t start=0;
    do {
        std::size_t end=text.find(';',start);
        std::string part=trim(text.substr(start,end==std::string::npos?end:end-start));
        if(part.compare(0,2,"x=")==0) part=part.substr(2);
        Radical value=evaluateExact(parseExpression(part));
        if(std::find(result.values.begin(),result.values.end(),value)==result.values.end()) result.values.push_back(value);
        if(result.values.size()>2) throw MathError("Не более двух корней",true);
        if(end==std::string::npos) break;
        start=end+1;
    } while(start<=text.size());
    return result;
}
}
CheckResult EquationChecker::checkAnswer(const TaskPart& task,const AnswerPayload& answer) const {
    if(task.kind==PartKind::Line || task.kind==PartKind::Parabola) return graphChecker.check(task,answer.graph);
    if(trim(answer.text).empty()) return CheckResult(CheckStatus::Incomplete,"Введите окончательный ответ");
    try {
        bool correct;
        if(task.kind==PartKind::Expression) correct=evaluateExact(parseExpression(task.expression))==evaluateExact(parseExpression(answer.text));
        else correct=sameRoots(solveEquation(task.expression),readRoots(answer.text));
        return CheckResult(correct?CheckStatus::Correct:CheckStatus::Incorrect,correct?"Верно":"Ответ не совпадает с решением");
    } catch(const MathError& error) { return errorResult(error); }
}
CheckResult EquationChecker::checkStep(const std::string& before,const std::string& after,const std::string& operation,const std::string& operand) const {
    if(trim(after).empty()) return CheckResult(CheckStatus::Incomplete,"Введите следующий шаг");
    try {
        Equation first=parseEquation(before), second=parseEquation(after);
        Polynomial left=polynomial(first.left),right=polynomial(first.right);
        Polynomial nextLeft=polynomial(second.left),nextRight=polynomial(second.right);
        if(!sameRoots(solve(left-right),solve(nextLeft-nextRight))) return CheckResult(CheckStatus::Incorrect,"Шаг изменил множество решений");
        if(!operation.empty()) {
            if(operation=="add" || operation=="subtract") {
                Polynomial value=polynomial(parseExpression(operand));
                if(operation=="add") { left=left+value; right=right+value; }
                else { left=left-value; right=right-value; }
            } else if(operation=="multiply" || operation=="divide") {
                Rational value=evaluateNumber(parseExpression(operand));
                if(value.zero()) return CheckResult(CheckStatus::Incorrect,"Нельзя умножать или делить обе части на ноль");
                if(operation=="divide") value=Rational(1)/value;
                left=left*Polynomial(value); right=right*Polynomial(value);
            } else if(operation=="swap") std::swap(left,right);
            else if(operation!="simplify") return CheckResult(CheckStatus::Unsupported,"Неизвестное действие");
            if(!(left==nextLeft) || !(right==nextRight)) return CheckResult(CheckStatus::Incorrect,"Уравнение равносильно, но заявленное действие не выполнено");
        }
        return CheckResult(CheckStatus::Correct,"Равносильный шаг");
    } catch(const MathError& error) { return errorResult(error); }
}
}
