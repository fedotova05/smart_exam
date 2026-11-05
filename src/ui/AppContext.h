#ifndef SMARTEXAM_APP_CONTEXT_H
#define SMARTEXAM_APP_CONTEXT_H
#include "application/SchoolService.h"
#include "io/packages/PackageService.h"
namespace exam {
namespace ui {
enum class UserRole { Guest, Student, Teacher };
struct AppContext {
    SchoolService &school;
    PackageService packages;
    std::string profile, profileName, page;
    SessionSettings settings;
    UserRole role;
    NextTask current;
    std::map<std::string, AnswerPayload> drafts;
    std::vector<std::string> practiceSkills;
    AppContext(SchoolService &service)
        : school(service), packages(service), role(UserRole::Guest) {}
    void selectProfile(const std::string &id, const std::string &name) {
        profile = id;
        profileName = name;
        current = NextTask();
        drafts.clear();
        practiceSkills.clear();
        settings = SessionSettings();
    }
};
bool canOpen(UserRole role, const std::string &page);
std::vector<std::string> splitCodes(const std::string &text);
std::string joinCodes(const std::vector<std::string> &codes);
std::string kindName(PartKind kind);
std::string resultText(const AttemptResult &result);
std::string scoreText(const AttemptResult &result, int maximum);
std::string skillEstimate(const SkillState &skill);
std::string skillEvidence(const SkillState &skill);
std::string lessonScore(const LessonProgress &progress);
std::string lessonNote(const LessonProgress &progress);
double readNumber(const std::string &text);
} // namespace ui
} // namespace exam
#endif
