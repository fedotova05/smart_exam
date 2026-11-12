#include "Html.h"
#include "Json.h"
#include <algorithm>
#include <cctype>
#include <map>
#include <stdexcept>
namespace exam {
namespace {
std::string lower(std::string s) {
    for (auto &c : s)
        c = char(std::tolower(static_cast<unsigned char>(c)));
    return s;
}
std::size_t tagEnd(const std::string &s, std::size_t p) {
    char quote = 0;
    for (; p < s.size(); ++p) {
        char c = s[p];
        if (quote) {
            if (c == quote)
                quote = 0;
        } else if (c == '\'' || c == '"')
            quote = c;
        else if (c == '>')
            return p;
    }
    throw std::runtime_error("Незавершённый тег в ответе источника");
}
std::string tagName(const std::string &s) {
    std::size_t a = 0;
    while (a < s.size() && (s[a] == '<' || s[a] == '/' || std::isspace((unsigned char)s[a])))
        ++a;
    auto b = a;
    while (b < s.size() && (std::isalnum((unsigned char)s[b]) || s[b] == '-'))
        ++b;
    return lower(s.substr(a, b - a));
}
void utf(std::string &s, unsigned n) {
    if (n == 0 || n > 0x10ffff || (n >= 0xd800 && n <= 0xdfff))
        throw std::runtime_error("Неверный символ HTML");
    if (n < 128)
        s += char(n);
    else if (n < 2048) {
        s += char(192 | (n >> 6));
        s += char(128 | (n & 63));
    } else if (n < 65536) {
        s += char(224 | (n >> 12));
        s += char(128 | ((n >> 6) & 63));
        s += char(128 | (n & 63));
    } else {
        s += char(240 | (n >> 18));
        s += char(128 | ((n >> 12) & 63));
        s += char(128 | ((n >> 6) & 63));
        s += char(128 | (n & 63));
    }
}
std::string decode(const std::string &s) {
    static const std::map<std::string, std::string> names = {
        {"amp", "&"},    {"lt", "<"},     {"gt", ">"},    {"quot", "\""}, {"apos", "'"},
        {"nbsp", " "},   {"thinsp", " "}, {"shy", ""},    {"times", "×"}, {"minus", "−"},
        {"middot", "·"}, {"ndash", "–"},  {"mdash", "—"}, {"le", "≤"},    {"ge", "≥"},
        {"ne", "≠"},     {"deg", "°"},    {"pi", "π"},    {"radic", "√"}, {"hellip", "…"}};
    std::string r;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '&') {
            auto e = s.find(';', i);
            if (e != std::string::npos && e - i < 16) {
                auto key = s.substr(i + 1, e - i - 1);
                auto v = names.find(key);
                if (v != names.end()) {
                    r += v->second;
                    i = e;
                    continue;
                }
                if (!key.empty() && key[0] == '#') {
                    std::size_t used = 0;
                    bool hex = key.size() > 1 && (key[1] == 'x' || key[1] == 'X');
                    auto digits = key.substr(hex ? 2 : 1);
                    try {
                        unsigned n = std::stoul(digits, &used, hex ? 16 : 10);
                        if (used == digits.size()) {
                            if (n != 173)
                                utf(r, (n == 160 || n == 8239 || n == 8201) ? 32 : n);
                            i = e;
                            continue;
                        }
                    } catch (...) {
                    }
                }
            }
        }
        if (s.compare(i, 2, "\302\255") == 0) {
            ++i;
            continue;
        }
        if (s.compare(i, 3, "\342\200\257") == 0 || s.compare(i, 3, "\342\200\211") == 0) {
            r += ' ';
            i += 2;
            continue;
        }
        if (s.compare(i, 2, "\302\240") == 0) {
            r += ' ';
            ++i;
            continue;
        }
        r += s[i];
    }
    return r;
}
} // namespace
std::string htmlAttribute(const std::string &s, const std::string &name) {
    std::size_t i = 0;
    while (i < s.size()) {
        while (i < s.size() && !(std::isalnum((unsigned char)s[i]) || s[i] == '_'))
            ++i;
        auto a = i;
        while (i < s.size() &&
               (std::isalnum((unsigned char)s[i]) || s[i] == '_' || s[i] == '-' || s[i] == ':'))
            ++i;
        auto key = lower(s.substr(a, i - a));
        while (i < s.size() && std::isspace((unsigned char)s[i]))
            ++i;
        if (i == s.size() || s[i] != '=')
            continue;
        ++i;
        while (i < s.size() && std::isspace((unsigned char)s[i]))
            ++i;
        char q = i < s.size() && (s[i] == '\'' || s[i] == '"') ? s[i++] : 0;
        a = i;
        while (i < s.size() && (q ? s[i] != q : !std::isspace((unsigned char)s[i]) && s[i] != '>'))
            ++i;
        auto value = s.substr(a, i - a);
        if (q && i < s.size())
            ++i;
        if (key == name)
            return decode(value);
    }
    return "";
}
std::vector<HtmlElement> htmlElements(const std::string &s, const std::string &wanted) {
    if (s.size() > 4 * 1024 * 1024)
        throw std::runtime_error("Страница слишком велика");
    std::vector<HtmlElement> result;
    for (std::size_t p = 0; (p = s.find('<', p)) != std::string::npos;) {
        if (s.compare(p, 4, "<!--") == 0) {
            auto e = s.find("-->", p);
            p = e == std::string::npos ? s.size() : e + 3;
            continue;
        }
        auto end = tagEnd(s, p + 1);
        auto tag = s.substr(p, end - p + 1);
        auto name = tagName(tag);
        auto cls = " " + htmlAttribute(tag, "class") + " ";
        bool match = cls.find(" " + wanted + " ") != std::string::npos;
        if (tag.size() > 1 && tag[1] == '/')
            match = false;
        if (!match) {
            p = end + 1;
            continue;
        }
        HtmlElement el;
        el.tag = name;
        el.attributes = tag;
        el.start = p;
        std::size_t at = end + 1;
        int depth = 1;
        for (; depth && (at = s.find('<', at)) != std::string::npos;) {
            auto close = tagEnd(s, at + 1);
            auto t = s.substr(at, close - at + 1);
            if (tagName(t) == name) {
                if (t.size() > 1 && t[1] == '/')
                    --depth;
                else if (t.size() < 2 || t[t.size() - 2] != '/')
                    ++depth;
            }
            if (!depth) {
                el.inner = s.substr(end + 1, at - end - 1);
                el.end = close + 1;
                break;
            }
            at = close + 1;
        }
        if (depth)
            throw std::runtime_error("Не удалось прочитать карточку источника");
        result.push_back(el);
        p = el.end;
        if (result.size() > 1000)
            throw std::runtime_error("Слишком много карточек");
    }
    return result;
}
std::vector<HtmlElement> htmlImages(const std::string &s) {
    std::vector<HtmlElement> result;
    for (std::size_t p = 0; (p = s.find('<', p)) != std::string::npos;) {
        auto e = tagEnd(s, p + 1);
        auto t = s.substr(p, e - p + 1);
        if (tagName(t) == "img") {
            HtmlElement el;
            el.tag = "img";
            el.attributes = t;
            el.start = p;
            el.end = e + 1;
            result.push_back(el);
        }
        p = e + 1;
    }
    return result;
}
std::string htmlText(const std::string &s) {
    std::string r;
    for (std::size_t i = 0; i < s.size();) {
        if (s[i] != '<') {
            r += s[i++];
            continue;
        }
        if (s.compare(i, 4, "<!--") == 0) {
            auto e = s.find("-->", i);
            i = e == std::string::npos ? s.size() : e + 3;
            continue;
        }
        auto e = tagEnd(s, i + 1);
        auto t = s.substr(i, e - i + 1);
        auto n = tagName(t);
        bool close = t.size() > 1 && t[1] == '/';
        if (n == "script" || n == "style") {
            auto end = s.find("</" + n, e);
            i = end == std::string::npos ? s.size() : tagEnd(s, end + 1) + 1;
            continue;
        }
        if (n == "img") {
            auto alt = htmlAttribute(t, "alt");
            r += alt.empty() ? " [Рисунок] " : " " + alt + " ";
        } else if (n == "p" || n == "br" || n == "tr" || n == "div" || n == "li")
            r += '\n';
        else if (n == "td" || n == "th")
            r += " | ";
        else if (n == "sup" && !close)
            r += "^(";
        else if (n == "sup" && close)
            r += ")";
        i = e + 1;
    }
    r = decode(r);
    std::string clean;
    bool space = false;
    for (char c : r) {
        if (c == '\r' || c == '\t' || c == ' ') {
            space = true;
            continue;
        }
        if (c == '\n') {
            while (!clean.empty() && clean.back() == ' ')
                clean.pop_back();
            if (!clean.empty() && clean.back() != '\n')
                clean += '\n';
            space = false;
            continue;
        }
        if (space && !clean.empty() && clean.back() != '\n')
            clean += ' ';
        clean += c;
        space = false;
    }
    while (!clean.empty() && std::isspace((unsigned char)clean.back()))
        clean.pop_back();
    if (!validUtf8(clean))
        throw std::runtime_error("Источник вернул неверную кодировку");
    return clean;
}
} // namespace exam
