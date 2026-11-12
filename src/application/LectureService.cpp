#include "SchoolService.h"
#include "io/Files.h"
#include <algorithm>
#include <set>
#include <cctype>
#include <windows.h>
#include <shellapi.h>
namespace exam {
namespace {
std::string attachmentPath(const std::string &name) {
    auto dot = name.find_last_of('.');
    std::string ext = dot == std::string::npos ? "" : name.substr(dot);
    if (ext != ".pdf" && ext != ".doc" && ext != ".docx")
        throw std::runtime_error("Допустимы PDF, DOC и DOCX");
    if (name.compare(0, 9, "lectures/") != 0 || !safeResourcePath(name.substr(0, dot) + ".png"))
        throw std::runtime_error("Недопустимый путь вложения");
    // Проверка каждого компонента также исключает ссылки на внешние каталоги.
    resourcePath("data", name.substr(0, dot) + ".png");
    std::string path = "data/" + name;
    int n = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, 0, 0);
    std::vector<wchar_t> wide(n);
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wide.data(), n);
    DWORD attributes = GetFileAttributesW(wide.data());
    if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT))
        throw std::runtime_error("Ссылки во вложениях запрещены");
    return path;
}
} // namespace
std::vector<Lecture> SchoolService::lectures(bool drafts) {
    return storage.lectures(drafts);
}
Lecture SchoolService::lecture(const std::string &id, bool drafts) {
    return storage.lecture(id, drafts);
}
void SchoolService::saveLecture(const Lecture &value, const std::string &file) {
    storage.requireTeacher();
    Lecture v = value;
    if (v.id.empty())
        v.id = newId("lecture");
    if (!file.empty()) {
        auto dot = file.find_last_of('.');
        std::string ext = dot == std::string::npos ? "" : file.substr(dot);
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char c) { return char(std::tolower(c)); });
        v.attachment = "lectures/" + newId("file") + ext;
        auto path = attachmentPath(v.attachment);
        auto bytes = readFile(file, 20 * 1024 * 1024);
        bool signature =
            (ext == ".pdf" && bytes.compare(0, 5, "%PDF-") == 0) ||
            (ext == ".docx" && bytes.compare(0, 4, "PK\003\004") == 0) ||
            (ext == ".doc" && bytes.compare(0, 8, "\320\317\021\340\241\261\032\341") == 0);
        if (!signature)
            throw std::runtime_error("Содержимое файла не соответствует расширению");
        makeDirectories("data/lectures");
        writeFileAtomic(path, bytes);
        v.attachmentName = file.substr(
            file.find_last_of("/\\") == std::string::npos ? 0 : file.find_last_of("/\\") + 1);
    }
    if (!v.attachment.empty())
        readFile(attachmentPath(v.attachment), 20 * 1024 * 1024);
    try {
        storage.saveLecture(v);
    } catch (...) {
        if (!file.empty()) {
            auto path = attachmentPath(v.attachment);
            int n = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, 0, 0);
            std::vector<wchar_t> wide(n);
            MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wide.data(), n);
            DeleteFileW(wide.data());
        }
        throw;
    }
}
std::vector<Lecture> SchoolService::recommendedLectures(const std::string &attempt) {
    auto a = storage.attempt(attempt);
    auto s = storage.session(a.sessionId);
    if (!s.settings.helpAllowed || !s.settings.training)
        return {};
    std::set<std::string> wanted;
    auto step = storage.trainingStep(attempt);
    if (!step.skill.empty())
        wanted.insert(step.skill);
    else
        for (const auto &p : storage.task(a.taskId).parts)
            wanted.insert(p.skills.begin(), p.skills.end());
    auto values = storage.lectures();
    std::vector<Lecture> result;
    for (const auto &v : values)
        for (const auto &id : v.skills)
            if (wanted.count(id)) {
                result.push_back(v);
                break;
            }
    auto graph = storage.skillGraph();
    std::set<std::string> parents;
    for (const auto &id : wanted)
        parents.insert(graph[id].begin(), graph[id].end());
    for (const auto &v : values) {
        if (std::find_if(result.begin(), result.end(),
                         [&](const Lecture &r) { return r.id == v.id; }) != result.end())
            continue;
        for (const auto &id : v.skills)
            if (parents.count(id)) {
                result.push_back(v);
                break;
            }
    }
    return result;
}
void SchoolService::readLecture(const std::string &id, const std::string &attempt) {
    storage.lecture(id);
    if (attempt.empty())
        return;
    if (!session)
        throw std::runtime_error("Сначала откройте занятие");
    auto a = storage.attempt(attempt);
    auto s = storage.session(a.sessionId);
    if (!s.settings.helpAllowed || !s.settings.training || a.finished)
        throw std::runtime_error("Лекции недоступны во время этой попытки");
    bool related = false;
    for (const auto &v : recommendedLectures(attempt))
        if (v.id == id)
            related = true;
    if (!related)
        throw std::runtime_error("Лекция не относится к текущему заданию");
    for (const auto &part : storage.task(a.taskId).parts)
        session->requestHelp("lecture-" + attempt + "-" + part.id + "-" + id, attempt, part.id,
                             "lecture:" + id);
}
void SchoolService::openLectureFile(const std::string &id, bool drafts) {
    auto v = storage.lecture(id, drafts);
    if (v.attachment.empty())
        throw std::runtime_error("У лекции нет вложения");
    std::string path = attachmentPath(v.attachment);
    readFile(path, 20 * 1024 * 1024);
    int n = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, 0, 0);
    std::vector<wchar_t> wide(n);
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wide.data(), n);
    auto result = ShellExecuteW(0, L"open", wide.data(), 0, 0, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= 32)
        throw std::runtime_error("Не удалось открыть файл. Установите программу для этого формата");
}
} // namespace exam
