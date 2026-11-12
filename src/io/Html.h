#ifndef SMARTEXAM_HTML_H
#define SMARTEXAM_HTML_H
#include <string>
#include <vector>
namespace exam {
struct HtmlElement {
    std::string tag, attributes, inner;
    std::size_t start = 0, end = 0;
};
std::string htmlAttribute(const std::string &tag, const std::string &name);
std::vector<HtmlElement> htmlElements(const std::string &html, const std::string &className);
std::string htmlText(const std::string &html);
std::vector<HtmlElement> htmlImages(const std::string &html);
} // namespace exam
#endif
