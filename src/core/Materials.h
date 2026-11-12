#ifndef SMARTEXAM_MATERIALS_H
#define SMARTEXAM_MATERIALS_H
#include "Types.h"
namespace exam {
struct Lecture {
    std::string id, title, body, attachment, attachmentName;
    std::vector<std::string> skills;
    bool published = false, archived = false;
    int revision = 0;
};
struct ExamTask {
    std::string taskId, exam = "oge", subject = "math", source = "sdamgia";
    std::string externalId, sourceUrl, fingerprint;
    int year = 2026, number = 0, category = 0;
};
} // namespace exam
#endif
