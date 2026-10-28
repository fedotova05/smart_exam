#include "Rational.h"
#include <limits>
#include <cstdlib>
namespace exam {
std::int64_t gcd(std::int64_t a, std::int64_t b) {
    if (a < 0)
        a = -a;
    if (b < 0)
        b = -b;
    while (b) {
        std::int64_t remainder = a % b;
        a = b;
        b = remainder;
    }
    return a;
}
Rational::Rational(std::int64_t n, std::int64_t d) {
    if (d == 0)
        throw MathError("Деление на ноль");
    if (n == std::numeric_limits<std::int64_t>::min() ||
        d == std::numeric_limits<std::int64_t>::min())
        throw MathError("Число слишком большое", true);
    if (d < 0) {
        n = -n;
        d = -d;
    }
    std::int64_t divisor = gcd(n, d);
    n /= divisor;
    d /= divisor;
    if (std::abs(n) > 1000000000LL || d > 1000000000LL)
        throw MathError("Превышен предел точных чисел", true);
    numeratorValue = n;
    denominatorValue = d;
}
Rational operator+(const Rational &a, const Rational &b) {
    // Каждый множитель <= 10^9; сумма двух произведений <= 2*10^18 < INT64_MAX.
    const std::int64_t divisor = gcd(a.denominator(), b.denominator());
    return Rational(a.numerator() * (b.denominator() / divisor) +
                        b.numerator() * (a.denominator() / divisor),
                    a.denominator() * (b.denominator() / divisor));
}
Rational operator-(const Rational &a) {
    return Rational(-a.numerator(), a.denominator());
}
Rational operator-(const Rational &a, const Rational &b) {
    return a + (-b);
}
Rational operator*(const Rational &a, const Rational &b) {
    const std::int64_t first = gcd(a.numerator(), b.denominator());
    const std::int64_t second = gcd(b.numerator(), a.denominator());
    return Rational((a.numerator() / first) * (b.numerator() / second),
                    (a.denominator() / second) * (b.denominator() / first));
}
Rational operator/(const Rational &a, const Rational &b) {
    if (b.zero())
        throw MathError("Деление на ноль");
    return a * Rational(b.denominator(), b.numerator());
}
bool operator==(const Rational &a, const Rational &b) {
    return a.numerator() == b.numerator() && a.denominator() == b.denominator();
}
bool operator!=(const Rational &a, const Rational &b) {
    return !(a == b);
}
bool operator<(const Rational &a, const Rational &b) {
    return a.numerator() * b.denominator() < b.numerator() * a.denominator();
}
std::string Rational::text() const {
    return std::to_string(numeratorValue) +
           (denominatorValue == 1 ? "" : "/" + std::to_string(denominatorValue));
}
double Rational::value() const {
    return static_cast<double>(numeratorValue) / denominatorValue;
}
Rational Rational::parse(const std::string &text) {
    if (text.empty())
        throw MathError("Ожидалось число");
    std::int64_t n = 0, d = 1;
    bool decimal = false;
    int digits = 0;
    for (char c : text) {
        if (c == '.' || c == ',') {
            if (decimal)
                throw MathError("Два разделителя в числе");
            decimal = true;
            continue;
        }
        if (c < '0' || c > '9')
            throw MathError("Неверная запись числа");
        if (n > (1000000000LL - (c - '0')) / 10)
            throw MathError("Число слишком большое", true);
        n = n * 10 + c - '0';
        ++digits;
        if (decimal) {
            if (d >= 1000000)
                throw MathError("Не более шести знаков после запятой", true);
            d *= 10;
        }
    }
    if (digits == 0 || text.back() == '.' || text.back() == ',')
        throw MathError("Незавершённое число");
    return Rational(n, d);
}
} // namespace exam
