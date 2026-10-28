#ifndef SMARTEXAM_RATIONAL_H
#define SMARTEXAM_RATIONAL_H
#include <cstdint>
#include <stdexcept>
#include <string>
namespace exam {
struct MathError : std::runtime_error {
    bool unsupported;
    int position;
    MathError(const std::string &text, bool limit = false, int at = -1)
        : std::runtime_error(text), unsupported(limit), position(at) {}
};
// После сокращения модуль числителя и знаменатель не превышают 10^9.
class Rational {
    std::int64_t numeratorValue, denominatorValue;

  public:
    Rational(std::int64_t numerator = 0, std::int64_t denominator = 1);
    std::int64_t numerator() const {
        return numeratorValue;
    }
    std::int64_t denominator() const {
        return denominatorValue;
    }
    bool zero() const {
        return numeratorValue == 0;
    }
    std::string text() const;
    double value() const;
    static Rational parse(const std::string &text);
};
Rational operator+(const Rational &a, const Rational &b);
Rational operator-(const Rational &a, const Rational &b);
Rational operator*(const Rational &a, const Rational &b);
Rational operator/(const Rational &a, const Rational &b);
Rational operator-(const Rational &a);
bool operator==(const Rational &a, const Rational &b);
bool operator!=(const Rational &a, const Rational &b);
bool operator<(const Rational &a, const Rational &b);
std::int64_t gcd(std::int64_t a, std::int64_t b);
} // namespace exam
#endif
