#ifndef SMARTEXAM_JSON_H
#define SMARTEXAM_JSON_H
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
namespace exam {
class Json {
  public:
    enum Type { Null, Boolean, Number, String, Array, Object };
    Type type;
    bool boolean;
    double number;
    std::string string;
    std::vector<Json> array;
    std::map<std::string, Json> object;
    Json() : type(Null), boolean(false), number(0) {}
    explicit Json(const std::string &value)
        : type(String), boolean(false), number(0), string(value) {}
    explicit Json(double value) : type(Number), boolean(false), number(value) {}
    static Json makeArray();
    static Json makeObject();
    const Json &field(const std::string &name) const;
    std::string text() const;
    int integer() const;
};
Json parseJson(const std::string &source);
std::string writeJson(const Json &value);
bool validUtf8(const std::string &text);
} // namespace exam
#endif
