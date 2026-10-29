#include "Json.h"
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
namespace exam {
Json Json::makeArray() {
    Json value;
    value.type = Array;
    return value;
}
Json Json::makeObject() {
    Json value;
    value.type = Object;
    return value;
}
const Json &Json::field(const std::string &name) const {
    if (type != Object || !object.count(name))
        throw std::runtime_error("Нет поля: " + name);
    return object.at(name);
}
std::string Json::text() const {
    if (type != String)
        throw std::runtime_error("Ожидалась строка JSON");
    return string;
}
int Json::integer() const {
    if (type != Number || !std::isfinite(number) || number != std::floor(number) ||
        number < std::numeric_limits<int>::min() || number > std::numeric_limits<int>::max())
        throw std::runtime_error("Ожидалось целое число JSON");
    return static_cast<int>(number);
}
bool validUtf8(const std::string &text) {
    for (std::size_t i = 0; i < text.size();) {
        unsigned char first = static_cast<unsigned char>(text[i++]);
        if (first < 128)
            continue;
        unsigned code;
        int count;
        if (first >= 0xC2 && first <= 0xDF) {
            code = first & 31;
            count = 1;
        } else if (first >= 0xE0 && first <= 0xEF) {
            code = first & 15;
            count = 2;
        } else if (first >= 0xF0 && first <= 0xF4) {
            code = first & 7;
            count = 3;
        } else
            return false;
        if (i + count > text.size())
            return false;
        for (int j = 0; j < count; ++j) {
            unsigned char next = static_cast<unsigned char>(text[i++]);
            if ((next & 0xC0) != 0x80)
                return false;
            code = (code << 6) | (next & 63);
        }
        if ((count == 1 && code < 128) || (count == 2 && code < 2048) ||
            (count == 3 && code < 65536) || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF))
            return false;
    }
    return true;
}
namespace {
void appendUtf8(std::string &text, unsigned code) {
    if (code < 128)
        text += static_cast<char>(code);
    else if (code < 2048) {
        text += static_cast<char>(0xC0 | (code >> 6));
        text += static_cast<char>(0x80 | (code & 63));
    } else if (code < 65536) {
        text += static_cast<char>(0xE0 | (code >> 12));
        text += static_cast<char>(0x80 | ((code >> 6) & 63));
        text += static_cast<char>(0x80 | (code & 63));
    } else {
        text += static_cast<char>(0xF0 | (code >> 18));
        text += static_cast<char>(0x80 | ((code >> 12) & 63));
        text += static_cast<char>(0x80 | ((code >> 6) & 63));
        text += static_cast<char>(0x80 | (code & 63));
    }
}
class JsonReader {
    const std::string &source;
    std::size_t position;
    int nodes;
    void fail(const std::string &message) const {
        throw std::runtime_error(message + " (позиция " + std::to_string(position) + ")");
    }
    void spaces() {
        while (position < source.size() && (source[position] == ' ' || source[position] == '\n' ||
                                            source[position] == '\r' || source[position] == '\t'))
            ++position;
    }
    char peek() const {
        return position < source.size() ? source[position] : '\0';
    }
    char take() {
        if (position >= source.size())
            fail("Неожиданный конец JSON");
        return source[position++];
    }
    void expect(char value) {
        if (take() != value)
            fail("Ожидался другой символ");
    }
    unsigned hex() {
        unsigned value = 0;
        for (int i = 0; i < 4; ++i) {
            char c = take();
            unsigned digit;
            if (c >= '0' && c <= '9')
                digit = c - '0';
            else if (c >= 'a' && c <= 'f')
                digit = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F')
                digit = c - 'A' + 10;
            else {
                fail("Неверный Unicode-код");
                return 0;
            }
            value = value * 16 + digit;
        }
        return value;
    }
    std::string readString() {
        expect('"');
        std::string text;
        while (peek() != '"') {
            unsigned char c = static_cast<unsigned char>(take());
            if (c < 32)
                fail("Управляющий символ в строке");
            if (c != '\\')
                text += static_cast<char>(c);
            else {
                char escaped = take();
                if (escaped == '"' || escaped == '\\' || escaped == '/')
                    text += escaped;
                else if (escaped == 'n')
                    text += '\n';
                else if (escaped == 'r')
                    text += '\r';
                else if (escaped == 't')
                    text += '\t';
                else if (escaped == 'b')
                    text += '\b';
                else if (escaped == 'f')
                    text += '\f';
                else if (escaped == 'u') {
                    unsigned code = hex();
                    if (code >= 0xD800 && code <= 0xDBFF) {
                        expect('\\');
                        expect('u');
                        unsigned low = hex();
                        if (low < 0xDC00 || low > 0xDFFF)
                            fail("Неверная Unicode-пара");
                        code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
                    } else if (code >= 0xDC00 && code <= 0xDFFF)
                        fail("Неполная Unicode-пара");
                    appendUtf8(text, code);
                } else
                    fail("Неизвестное экранирование");
            }
            if (text.size() > 65536)
                fail("Слишком длинная строка");
        }
        expect('"');
        return text;
    }
    Json readNumber() {
        std::size_t start = position;
        if (peek() == '-')
            take();
        if (peek() == '0')
            take();
        else {
            if (peek() < '1' || peek() > '9')
                fail("Неверное число");
            while (peek() >= '0' && peek() <= '9')
                take();
        }
        if (peek() == '.') {
            take();
            if (peek() < '0' || peek() > '9')
                fail("Незавершённая дробь");
            while (peek() >= '0' && peek() <= '9')
                take();
        }
        if (peek() == 'e' || peek() == 'E') {
            take();
            if (peek() == '+' || peek() == '-')
                take();
            if (peek() < '0' || peek() > '9')
                fail("Неверная степень числа");
            while (peek() >= '0' && peek() <= '9')
                take();
        }
        std::istringstream input(source.substr(start, position - start));
        input.imbue(std::locale::classic());
        double number;
        if (!(input >> number) || !std::isfinite(number))
            fail("Число слишком большое");
        return Json(number);
    }
    Json readValue(int depth) {
        if (depth > 32 || ++nodes > 20000)
            fail("Превышены пределы JSON");
        spaces();
        char c = peek();
        if (c == '"')
            return Json(readString());
        if (c == '-' || (c >= '0' && c <= '9'))
            return readNumber();
        if (c == '[') {
            take();
            Json value = Json::makeArray();
            spaces();
            if (peek() != ']')
                while (true) {
                    value.array.push_back(readValue(depth + 1));
                    spaces();
                    if (peek() != ',')
                        break;
                    take();
                }
            expect(']');
            return value;
        }
        if (c == '{') {
            take();
            Json value = Json::makeObject();
            spaces();
            if (peek() != '}')
                while (true) {
                    spaces();
                    std::string key = readString();
                    spaces();
                    expect(':');
                    if (value.object.count(key))
                        fail("Повтор ключа: " + key);
                    value.object[key] = readValue(depth + 1);
                    spaces();
                    if (peek() != ',')
                        break;
                    take();
                }
            expect('}');
            return value;
        }
        for (const std::string &word : {"true", "false", "null"}) {
            if (source.compare(position, word.size(), word) == 0) {
                position += word.size();
                Json value;
                if (word != "null") {
                    value.type = Json::Boolean;
                    value.boolean = word == "true";
                }
                return value;
            }
        }
        fail("Ожидалось значение JSON");
        return Json();
    }

  public:
    explicit JsonReader(const std::string &text) : source(text), position(0), nodes(0) {}
    Json read() {
        Json value = readValue(0);
        spaces();
        if (position != source.size())
            fail("Лишний текст после JSON");
        return value;
    }
};
std::string escaped(const std::string &text) {
    if (!validUtf8(text))
        throw std::runtime_error("Неверная строка UTF-8");
    std::ostringstream result;
    result << '"';
    for (unsigned char c : text) {
        if (c == '"' || c == '\\')
            result << '\\' << static_cast<char>(c);
        else if (c < 32)
            result << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                   << static_cast<unsigned>(c) << std::dec;
        else
            result << static_cast<char>(c);
    }
    result << '"';
    return result.str();
}
std::string writeValue(const Json &value, int depth) {
    if (depth > 32)
        throw std::runtime_error("Слишком глубокий JSON");
    if (value.type == Json::Null)
        return "null";
    if (value.type == Json::Boolean)
        return value.boolean ? "true" : "false";
    if (value.type == Json::String)
        return escaped(value.string);
    if (value.type == Json::Number) {
        if (!std::isfinite(value.number))
            throw std::runtime_error("Неверное число JSON");
        std::ostringstream text;
        text.imbue(std::locale::classic());
        text << std::setprecision(17) << value.number;
        return text.str();
    }
    std::string text = value.type == Json::Array ? "[" : "{";
    bool first = true;
    if (value.type == Json::Array)
        for (const Json &item : value.array) {
            if (!first)
                text += ",";
            text += writeValue(item, depth + 1);
            first = false;
        }
    else
        for (const auto &item : value.object) {
            if (!first)
                text += ",";
            text += escaped(item.first) + ":" + writeValue(item.second, depth + 1);
            first = false;
        }
    return text + (value.type == Json::Array ? "]" : "}");
}
} // namespace
Json parseJson(const std::string &source) {
    if (source.size() > 2 * 1024 * 1024)
        throw std::runtime_error("JSON больше 2 МБ");
    if (!validUtf8(source))
        throw std::runtime_error("Файл должен быть в UTF-8");
    return JsonReader(source).read();
}
std::string writeJson(const Json &value) {
    return writeValue(value, 0);
}
} // namespace exam
