#ifndef SMARTEXAM_FILES_H
#define SMARTEXAM_FILES_H
#include <string>
#include <cstddef>
namespace exam {
std::string readFile(const std::string &path, std::size_t maxBytes = 2 * 1024 * 1024);
void writeFile(const std::string &path, const std::string &bytes);
void makeDirectories(const std::string &path);
std::string parentPath(const std::string &path);
bool fileExists(const std::string &path);
bool safeResourcePath(const std::string &path);
std::string resourcePath(const std::string &base, const std::string &relative);
std::string newId(const std::string &prefix);
} // namespace exam
#endif
