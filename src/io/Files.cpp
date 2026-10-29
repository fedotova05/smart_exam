#include "Files.h"
#include <windows.h>
#include <cstdio>
#include <cctype>
#include <sstream>
#include <stdexcept>
namespace exam {
namespace {
std::wstring wide(const std::string &text) {
    int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.c_str(), -1, 0, 0);
    if (size == 0)
        throw std::runtime_error("Неверный путь UTF-8");
    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &result[0], size);
    result.resize(size - 1);
    return result;
}
} // namespace
std::string readFile(const std::string &path, std::size_t maxBytes) {
    HANDLE file = CreateFileW(wide(path).c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE)
        throw std::runtime_error("Не удалось открыть файл: " + path);
    std::string bytes;
    char buffer[4096];
    DWORD count = 0;
    bool failed = false;
    while (true) {
        if (!ReadFile(file, buffer, sizeof(buffer), &count, 0)) {
            failed = true;
            break;
        }
        if (count == 0)
            break;
        if (bytes.size() + count > maxBytes) {
            CloseHandle(file);
            throw std::runtime_error("Файл превышает допустимый размер");
        }
        bytes.append(buffer, count);
    }
    CloseHandle(file);
    if (failed)
        throw std::runtime_error("Ошибка чтения файла");
    return bytes;
}
void writeFile(const std::string &path, const std::string &bytes) {
    HANDLE file = CreateFileW(wide(path).c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE)
        throw std::runtime_error("Не удалось создать файл: " + path);
    DWORD count = 0;
    bool failed = !WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &count, 0) ||
                  count != bytes.size();
    if (!CloseHandle(file))
        failed = true;
    if (failed)
        throw std::runtime_error("Ошибка записи файла");
}
std::string parentPath(const std::string &path) {
    std::size_t slash = path.find_last_of("/\\");
    return slash == std::string::npos ? "." : path.substr(0, slash);
}
void makeDirectories(const std::string &path) {
    if (path.empty() || path == "." || (path.size() == 2 && path[1] == ':'))
        return;
    DWORD attributes = GetFileAttributesW(wide(path).c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES) {
        if (!(attributes & FILE_ATTRIBUTE_DIRECTORY))
            throw std::runtime_error("Ожидалась папка");
        return;
    }
    std::string parent = parentPath(path);
    if (parent != path)
        makeDirectories(parent);
    if (!CreateDirectoryW(wide(path).c_str(), 0) && GetLastError() != ERROR_ALREADY_EXISTS)
        throw std::runtime_error("Не удалось создать папку: " + path);
}
bool fileExists(const std::string &path) {
    return GetFileAttributesW(wide(path).c_str()) != INVALID_FILE_ATTRIBUTES;
}
bool safeResourcePath(const std::string &path) {
    if (path.empty() || path.size() > 200 || path[0] == '/' || path[0] == '\\')
        return false;
    std::string segment;
    for (std::size_t i = 0; i <= path.size(); ++i) {
        char c = i == path.size() ? '/' : path[i];
        if (c == '/') {
            if (segment.empty() || segment == "." || segment == ".." || segment.back() == '.')
                return false;
            std::string name = segment.substr(0, segment.find('.'));
            for (char &letter : name)
                letter = static_cast<char>(std::tolower(static_cast<unsigned char>(letter)));
            if (name == "con" || name == "prn" || name == "aux" || name == "nul" ||
                name.compare(0, 3, "com") == 0 || name.compare(0, 3, "lpt") == 0)
                return false;
            segment.clear();
        } else {
            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                  c == '_' || c == '-' || c == '.'))
                return false;
            segment += c;
        }
    }
    std::string extension = path.substr(
        path.find_last_of('.') == std::string::npos ? path.size() : path.find_last_of('.'));
    for (char &letter : extension)
        letter = static_cast<char>(std::tolower(static_cast<unsigned char>(letter)));
    return extension == ".png" || extension == ".jpg" || extension == ".jpeg";
}
std::string resourcePath(const std::string &base, const std::string &relative) {
    if (!safeResourcePath(relative))
        throw std::runtime_error("Недопустимый путь ресурса: " + relative);
    std::string result = base;
    std::size_t start = 0;
    while (start < relative.size()) {
        std::size_t slash = relative.find('/', start);
        result += "/" + relative.substr(start, slash == std::string::npos ? slash : slash - start);
        DWORD attributes = GetFileAttributesW(wide(result).c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT))
            throw std::runtime_error("Ссылки в ресурсах запрещены");
        if (slash == std::string::npos)
            break;
        start = slash + 1;
    }
    return result;
}
std::string newId(const std::string &prefix) {
    FILETIME time;
    GetSystemTimeAsFileTime(&time);
    static unsigned counter = 0;
    std::ostringstream result;
    result << prefix << '-' << std::hex << time.dwHighDateTime << '-' << time.dwLowDateTime << '-'
           << GetCurrentProcessId() << '-' << ++counter;
    return result.str();
}
} // namespace exam
