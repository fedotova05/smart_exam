#include "SvgImage.h"
#include "Files.h"
#include "Html.h"
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <vector>
#ifndef JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
#define JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE 0x00002000
#endif
namespace exam {
namespace {
std::wstring wide(const std::string &text) {
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.c_str(), -1, 0, 0);
    if (!n)
        throw std::runtime_error("Неверный путь UTF-8");
    std::wstring result(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &result[0], n);
    result.resize(n - 1);
    return result;
}
std::wstring absolute(const std::string &path) {
    std::vector<wchar_t> output(32768);
    if (!GetFullPathNameW(wide(path).c_str(), DWORD(output.size()), output.data(), 0))
        throw std::runtime_error("Не удалось определить путь рисунка");
    return output.data();
}
struct TemporarySvg {
    std::string input, output;
    TemporarySvg() {
        makeDirectories("data/imports");
        input = "data/imports/" + newId("svg") + ".svg";
        output = input + ".png";
    }
    ~TemporarySvg() {
        DeleteFileW(wide(input).c_str());
        DeleteFileW(wide(output).c_str());
    }
};
struct Process {
    HANDLE process = 0, thread = 0, job = 0;
    ~Process() {
        if (job)
            CloseHandle(job);
        if (thread)
            CloseHandle(thread);
        if (process)
            CloseHandle(process);
    }
};
} // namespace
std::string svgToPng(const std::string &bytes) {
    if (bytes.size() > 512 * 1024 || bytes.find('\0') != std::string::npos)
        throw std::runtime_error("SVG превышает допустимый размер");
    std::string lower = bytes;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return char(std::tolower(c)); });
    if (lower.find("<!entity") != std::string::npos)
        throw std::runtime_error("Внешние сущности в SVG запрещены");
    for (std::size_t at = 0; (at = bytes.find('<', at)) != std::string::npos;) {
        auto end = bytes.find('>', at);
        if (end == std::string::npos)
            throw std::runtime_error("Незавершённый SVG");
        auto tag = bytes.substr(at, end - at + 1);
        for (const auto &name : {"href", "xlink:href"}) {
            auto value = htmlAttribute(tag, name);
            if (!value.empty() && value[0] != '#')
                throw std::runtime_error("Внешние ресурсы в SVG запрещены");
        }
        at = end + 1;
    }
    for (std::size_t at = 0; (at = lower.find("url(", at)) != std::string::npos;) {
        at += 4;
        while (at < lower.size() &&
               (std::isspace((unsigned char)lower[at]) || lower[at] == '\'' || lower[at] == '"'))
            ++at;
        if (at == lower.size() || lower[at] != '#')
            throw std::runtime_error("Внешние ресурсы в SVG запрещены");
    }
    TemporarySvg files;
    writeFile(files.input, bytes);
    auto executable = absolute("third_party/resvg/resvg.exe");
    wchar_t windows[MAX_PATH];
    GetWindowsDirectoryW(windows, MAX_PATH);
    std::wstring command = L"\"" + executable +
                           L"\" --quiet --width 1600 --height 1000 --skip-system-fonts "
                           L"--font-family Arial --use-font-file \"" +
                           std::wstring(windows) + L"\\Fonts\\arial.ttf\" \"" +
                           absolute(files.input) + L"\" \"" + absolute(files.output) + L"\"";
    STARTUPINFOW startup = {};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION info = {};
    Process child;
    child.job = CreateJobObjectW(0, 0);
    if (!child.job)
        throw std::runtime_error("Не удалось выделить процесс отрисовки");
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
    limits.BasicLimitInformation.LimitFlags =
        JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE | JOB_OBJECT_LIMIT_PROCESS_MEMORY;
    limits.ProcessMemoryLimit = 256 * 1024 * 1024;
    if (!SetInformationJobObject(child.job, JobObjectExtendedLimitInformation, &limits,
                                 sizeof(limits)))
        throw std::runtime_error("Не удалось ограничить память отрисовки");
    if (!CreateProcessW(executable.c_str(), &command[0], 0, 0, FALSE,
                        CREATE_NO_WINDOW | CREATE_SUSPENDED, 0, 0, &startup, &info))
        throw std::runtime_error(
            "Не удалось запустить SVG-рендерер. Нужен resvg из комплекта приложения и Windows x64");
    child.process = info.hProcess;
    child.thread = info.hThread;
    if (!AssignProcessToJobObject(child.job, child.process)) {
        TerminateProcess(child.process, 1);
        throw std::runtime_error("Не удалось ограничить процесс отрисовки");
    }
    ResumeThread(child.thread);
    if (WaitForSingleObject(child.process, 5000) != WAIT_OBJECT_0) {
        TerminateJobObject(child.job, 1);
        throw std::runtime_error("Отрисовка SVG превысила 5 секунд");
    }
    DWORD exitCode = 1;
    GetExitCodeProcess(child.process, &exitCode);
    if (exitCode)
        throw std::runtime_error("Не удалось отрисовать SVG");
    auto png = readFile(files.output, 8 * 1024 * 1024);
    if (png.compare(0, 8, "\211PNG\r\n\032\n") != 0)
        throw std::runtime_error("Рендерер не вернул PNG");
    return png;
}
} // namespace exam
