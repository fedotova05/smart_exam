#ifndef SMARTEXAM_HTTPS_H
#define SMARTEXAM_HTTPS_H
#include <functional>
#include <string>
namespace exam {
std::string downloadExamUrl(const std::string &url, const std::function<void()> &checkpoint = {},
                            std::size_t limit = 4 * 1024 * 1024);
std::string examResourceUrl(const std::string &url);
} // namespace exam
#endif
