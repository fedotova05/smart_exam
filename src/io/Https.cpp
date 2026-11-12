#include "Https.h"
#include <windows.h>
#include "WinHttpApi.h"
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>
#ifndef WINHTTP_OPTION_DECOMPRESSION
#define WINHTTP_OPTION_DECOMPRESSION 118
#endif
#define WINHTTP_ACCESS_TYPE_DEFAULT_PROXY 0
#define WINHTTP_NO_PROXY_NAME 0
#define WINHTTP_NO_PROXY_BYPASS 0
#define WINHTTP_OPTION_SECURE_PROTOCOLS 84
#define INTERNET_DEFAULT_HTTPS_PORT 443
#define WINHTTP_NO_REFERER 0
#define WINHTTP_DEFAULT_ACCEPT_TYPES 0
#define WINHTTP_FLAG_SECURE 0x00800000
#define WINHTTP_DISABLE_REDIRECTS 2
#define WINHTTP_OPTION_DISABLE_FEATURE 63
#define WINHTTP_NO_REQUEST_DATA 0
#define WINHTTP_QUERY_STATUS_CODE 19
#define WINHTTP_QUERY_FLAG_NUMBER 0x20000000
#define WINHTTP_HEADER_NAME_BY_INDEX 0
#define WINHTTP_NO_HEADER_INDEX 0
#define WINHTTP_QUERY_LOCATION 33
namespace exam {
namespace {
struct CloseHttp {
    void operator()(void *h) const {
        if (h)
            httpApi().CloseHandle(h);
    }
};
typedef std::unique_ptr<void, CloseHttp> Http;
std::wstring wide(const std::string &s) {
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.c_str(), -1, 0, 0);
    if (!n)
        throw std::runtime_error("Неверный адрес");
    std::wstring w(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], n);
    w.resize(n - 1);
    return w;
}
std::string narrow(const std::wstring &w) {
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, 0, 0, 0, 0);
    std::string s(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &s[0], n, 0, 0);
    s.resize(n - 1);
    return s;
}
void check(BOOL ok) {
    if (!ok)
        throw std::runtime_error("Источник недоступен (сетевая ошибка " +
                                 std::to_string(GetLastError()) +
                                 "). Сохранённые задания доступны без интернета");
}
std::string host(const std::string &url) {
    if (url.compare(0, 8, "https://") != 0 || url.size() > 2048 ||
        url.find_first_of("\r\n\\") != std::string::npos)
        throw std::runtime_error("Ожидался HTTPS-адрес источника");
    auto end = url.find('/', 8);
    std::string name = url.substr(8, end == std::string::npos ? end : end - 8);
    static const std::set<std::string> allowed = {"math-oge.sdamgia.ru", "math.oge.sdamgia.ru",
                                                  "oge.sdamgia.ru",      "ege-fipi.ru",
                                                  "oge.fipi.ru",         "doc.fipi.ru"};
    if (!allowed.count(name))
        throw std::runtime_error("Адрес не относится к источникам ОГЭ");
    return name;
}
} // namespace
std::string examResourceUrl(const std::string &url) {
    std::string result = url;
    if (url.compare(0, 2, "//") == 0)
        result = "https:" + url;
    else if (!url.empty() && url[0] == '/')
        result = "https://math-oge.sdamgia.ru" + url;
    host(result);
    return result;
}
std::string downloadExamUrl(const std::string &input, const std::function<void()> &checkpoint,
                            std::size_t limit) {
    std::string url = input;
    for (int redirect = 0; redirect < 4; ++redirect) {
        if (checkpoint)
            checkpoint();
        auto server = wide(host(url));
        auto pathAt = url.find('/', 8);
        auto path = wide(pathAt == std::string::npos ? "/" : url.substr(pathAt));
        Http session(httpApi().Open(L"SmartExam/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
        check(bool(session));
        check(httpApi().SetTimeouts(session.get(), 4000, 4000, 5000, 5000));
        DWORD protocols = 0x00000800;
        httpApi().SetOption(session.get(), WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols,
                            sizeof(protocols));
        Http connection(
            httpApi().Connect(session.get(), server.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0));
        check(bool(connection));
        Http request(httpApi().OpenRequest(connection.get(), L"GET", path.c_str(), 0,
                                           WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                           WINHTTP_FLAG_SECURE));
        check(bool(request));
        DWORD decompression = 3;
        check(httpApi().SetOption(request.get(), WINHTTP_OPTION_DECOMPRESSION, &decompression,
                                  sizeof(decompression)));
        DWORD disabled = WINHTTP_DISABLE_REDIRECTS;
        check(httpApi().SetOption(request.get(), WINHTTP_OPTION_DISABLE_FEATURE, &disabled,
                                  sizeof(disabled)));
        check(httpApi().SendRequest(request.get(), L"Accept-Language: ru-RU\r\n", -1L,
                                    WINHTTP_NO_REQUEST_DATA, 0, 0, 0));
        check(httpApi().ReceiveResponse(request.get(), 0));
        if (checkpoint)
            checkpoint();
        DWORD code = 0, bytes = sizeof(code);
        check(httpApi().QueryHeaders(
            request.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &code, &bytes, WINHTTP_NO_HEADER_INDEX));
        if (code >= 300 && code < 400) {
            wchar_t location[2048];
            bytes = sizeof(location);
            check(httpApi().QueryHeaders(request.get(), WINHTTP_QUERY_LOCATION,
                                         WINHTTP_HEADER_NAME_BY_INDEX, location, &bytes,
                                         WINHTTP_NO_HEADER_INDEX));
            std::string next = narrow(location);
            url = next.empty() || next[0] != '/' ? next : "https://" + host(url) + next;
            host(url);
            continue;
        }
        if (code != 200)
            throw std::runtime_error("Источник вернул HTTP " + std::to_string(code));
        std::string result;
        char buffer[16384];
        DWORD got;
        do {
            if (checkpoint)
                checkpoint();
            check(httpApi().ReadData(request.get(), buffer, sizeof(buffer), &got));
            if (result.size() + got > limit)
                throw std::runtime_error("Ответ источника превышает допустимый размер");
            result.append(buffer, got);
        } while (got);
        return result;
    }
    throw std::runtime_error("Слишком много перенаправлений источника");
}
} // namespace exam
