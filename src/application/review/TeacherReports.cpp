#include "application/SchoolService.h"
namespace exam {
namespace {
std::string statusName(CheckStatus status) {
    if (status == CheckStatus::Correct)
        return "Верно";
    if (status == CheckStatus::Incorrect)
        return "Неверно";
    return "Нужна проверка";
}
std::string csvCell(std::string value) {
    // Табличные редакторы не должны выполнять текст ответа как формулу.
    std::size_t first = value.find_first_not_of(" \t\r\n");
    if ((!value.empty() && (value[0] == '\t' || value[0] == '\r' || value[0] == '\n')) ||
        (first != std::string::npos && std::string("=+-@").find(value[first]) != std::string::npos))
        value.insert(0, "'");
    std::string result = "\"";
    for (char c : value) {
        result += c;
        if (c == '"')
            result += '"';
    }
    return result + "\"";
}
} // namespace
std::vector<ReviewItem> SchoolService::reviewItems(const std::string &student, bool pendingOnly) {
    storage.requireTeacher();
    std::vector<ReviewItem> result;
    auto profiles = storage.profiles();
    auto archived = storage.archivedProfiles();
    profiles.insert(profiles.end(), archived.begin(), archived.end());
    for (const auto &profile : profiles) {
        if (!student.empty() && profile.first != student)
            continue;
        for (const auto &lesson : storage.studentSessions(profile.first)) {
            for (const auto &attempt : storage.attempts(lesson.id)) {
                if (!attempt.finished)
                    continue;
                auto task = storage.task(attempt.taskId);
                auto responses = storage.responses(attempt.id);
                auto verdicts = storage.reviewHistory(attempt.id);
                for (const auto &part : task.parts) {
                    ReviewItem item;
                    item.student = profile.first;
                    item.studentName = profile.second;
                    item.lesson = lesson.id;
                    item.attempt = attempt.id;
                    item.task = task.id;
                    item.title = task.title;
                    item.part = part.id;
                    item.automatic = CheckStatus::Incomplete;
                    for (const auto &response : responses)
                        if (response.partId == part.id && response.finalAnswer)
                            item.automatic = response.result.status;
                    item.effective = item.automatic;
                    item.reviewed = false;
                    for (const auto &verdict : verdicts)
                        if (verdict.partId == part.id) {
                            item.reviewed = true;
                            item.effective = verdict.status;
                        }
                    bool pending = item.effective != CheckStatus::Correct &&
                                   item.effective != CheckStatus::Incorrect;
                    if (!pendingOnly || pending)
                        result.push_back(item);
                }
            }
        }
    }
    return result;
}
std::vector<ReviewVerdict> SchoolService::reviewHistory(const std::string &attempt) {
    storage.requireTeacher();
    return storage.reviewHistory(attempt);
}
std::vector<ResponseRecord> SchoolService::reviewResponses(const std::string &attempt) {
    storage.requireTeacher();
    return storage.responses(attempt);
}
void SchoolService::revise(const ReviewVerdict &verdict) {
    storage.requireTeacher();
    storage.reviewPart(verdict);
}
std::string SchoolService::reportCsv(const std::string &student, bool pendingOnly) {
    std::string result = "\xEF\xBB\xBF";
    result += "Ученик;Профиль;Занятие;Попытка;Задание;Версия;Часть;Ответ;График;Автопроверка;"
              "Текущая оценка;Преподаватель;Причина\r\n";
    for (const auto &item : reviewItems(student, pendingOnly)) {
        std::string answer, graph, reviewer, reason;
        for (const auto &response : storage.responses(item.attempt))
            if (response.partId == item.part && response.finalAnswer) {
                answer = response.answer.text;
                graph = response.graph;
            }
        for (const auto &verdict : storage.reviewHistory(item.attempt))
            if (verdict.partId == item.part) {
                reviewer = verdict.reviewer;
                reason = verdict.reason;
            }
        std::vector<std::string> cells = {item.studentName,
                                          item.student,
                                          item.lesson,
                                          item.attempt,
                                          item.title,
                                          item.task,
                                          item.part,
                                          answer,
                                          graph,
                                          statusName(item.automatic),
                                          statusName(item.effective),
                                          reviewer,
                                          reason};
        for (std::size_t i = 0; i < cells.size(); ++i) {
            if (i)
                result += ';';
            result += csvCell(cells[i]);
        }
        result += "\r\n";
    }
    return result;
}
} // namespace exam
