#include "Parser.h"
#include <cctype>
namespace exam {
std::vector<Token> tokenize(const std::string &text) {
    if (text.size() > 512)
        throw MathError("Не более 512 символов", true, 512);
    std::vector<Token> tokens;
    for (std::size_t i = 0; i < text.size();) {
        if (std::isspace(static_cast<unsigned char>(text[i]))) {
            ++i;
            continue;
        }
        std::size_t start = i;
        char kind = text[i];
        if (std::isdigit(static_cast<unsigned char>(text[i])) || text[i] == '.' || text[i] == ',') {
            kind = 'n';
            while (i < text.size() && (std::isdigit(static_cast<unsigned char>(text[i])) ||
                                       text[i] == '.' || text[i] == ','))
                ++i;
        } else if (text.compare(i, 4, "sqrt") == 0) {
            kind = 's';
            i += 4;
        } else if (std::string("x+-*/^()=").find(kind) != std::string::npos)
            ++i;
        else
            throw MathError("Неизвестный символ", false, static_cast<int>(i));
        tokens.push_back(Token{kind, text.substr(start, i - start), static_cast<int>(start)});
        if (tokens.size() > 256)
            throw MathError("Слишком много элементов", true, static_cast<int>(start));
    }
    tokens.push_back(Token{'\0', "", static_cast<int>(text.size())});
    return tokens;
}
namespace {
class Parser {
    std::vector<Token> tokens;
    std::size_t index;
    int depth;
    Expr node(NodeKind kind, int position, Expr left = Expr(), Expr right = Expr()) {
        Expr value(new Expression(kind, position));
        value->left = left;
        value->right = right;
        return value;
    }
    Token take() {
        return tokens[index++];
    }
    char peek() const {
        return tokens[index].kind;
    }
    void expect(char c) {
        if (peek() != c)
            throw MathError("Ожидался символ " + std::string(1, c), false, tokens[index].position);
        take();
    }
    Expr sum() {
        Expr value = product();
        while (peek() == '+' || peek() == '-') {
            Token op = take();
            Expr right = product();
            value = node(op.kind == '+' ? NodeKind::Add : NodeKind::Subtract, op.position, value,
                         right);
        }
        return value;
    }
    Expr product() {
        Expr value = unary();
        while (true) {
            char c = peek();
            bool implicit = c == 'x' || c == '(' || c == 's';
            if (c != '*' && c != '/' && !implicit)
                break;
            Token op = tokens[index];
            if (!implicit)
                take();
            Expr right = unary();
            value =
                node(c == '/' ? NodeKind::Divide : NodeKind::Multiply, op.position, value, right);
        }
        return value;
    }
    Expr unary() {
        if (++depth > 64)
            throw MathError("Слишком глубокое выражение", true, tokens[index].position);
        Expr value;
        if (peek() == '+' || peek() == '-') {
            Token op = take();
            value = unary();
            if (op.kind == '-')
                value = node(NodeKind::Negate, op.position, value);
        } else
            value = power();
        --depth;
        return value;
    }
    Expr power() {
        Expr value = primary();
        if (peek() == '^') {
            Token op = take();
            Expr right = unary();
            value = node(NodeKind::Power, op.position, value, right);
        }
        return value;
    }
    Expr primary() {
        Token token = take();
        if (token.kind == 'n') {
            Expr value = node(NodeKind::Number, token.position);
            try {
                value->number = Rational::parse(token.text);
            } catch (const MathError &error) {
                throw MathError(error.what(), error.unsupported, token.position);
            }
            return value;
        }
        if (token.kind == 'x')
            return node(NodeKind::Variable, token.position);
        if (token.kind == '(') {
            Expr value = sum();
            expect(')');
            return value;
        }
        if (token.kind == 's') {
            expect('(');
            Expr value = sum();
            expect(')');
            return node(NodeKind::SquareRoot, token.position, value);
        }
        throw MathError("Ожидалось число, x или скобка", false, token.position);
    }

  public:
    explicit Parser(const std::string &text) : tokens(tokenize(text)), index(0), depth(0) {}
    Expr expression() {
        Expr value = sum();
        if (peek() != 0)
            throw MathError("Лишний элемент", false, tokens[index].position);
        return value;
    }
    Equation equation() {
        Equation value;
        value.left = sum();
        expect('=');
        value.right = sum();
        if (peek() != 0)
            throw MathError("Лишний элемент", false, tokens[index].position);
        return value;
    }
};
} // namespace
Expr parseExpression(const std::string &text) {
    return Parser(text).expression();
}
Equation parseEquation(const std::string &text) {
    return Parser(text).equation();
}
Rational evaluateNumber(const Expr &e) {
    if (e->kind == NodeKind::Number)
        return e->number;
    if (e->kind == NodeKind::Variable || e->kind == NodeKind::SquareRoot)
        throw MathError("Ожидалось рациональное число", true, e->position);
    Rational left = evaluateNumber(e->left);
    if (e->kind == NodeKind::Negate)
        return -left;
    Rational right = evaluateNumber(e->right);
    switch (e->kind) {
    case NodeKind::Add:
        return left + right;
    case NodeKind::Subtract:
        return left - right;
    case NodeKind::Multiply:
        return left * right;
    case NodeKind::Divide:
        return left / right;
    case NodeKind::Power: {
        if (right.denominator() != 1 || right.numerator() < 0 || right.numerator() > 8)
            throw MathError("Степень должна быть целой от 0 до 8", true, e->position);
        if (left.zero() && right.zero())
            throw MathError("Неопределённость 0^0", false, e->position);
        Rational result(1);
        for (int i = 0; i < right.numerator(); ++i)
            result = result * left;
        return result;
    }
    default:
        throw MathError("Неподдерживаемая операция", true, e->position);
    }
}
} // namespace exam
