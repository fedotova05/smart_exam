#ifndef SMARTEXAM_APP_CONTEXT_H
#define SMARTEXAM_APP_CONTEXT_H
#include "application/SchoolService.h"
#include "io/packages/PackageService.h"
namespace exam {
namespace ui {
struct AppContext {
    SchoolService &school;
    PackageService packages;
    std::string profile, profileName, page;
    SessionSettings settings;
    NextTask current;
    std::map<std::string, AnswerPayload> drafts;
    AppContext(SchoolService &service) : school(service), packages(service) {}
};
std::vector<std::string> splitCodes(const std::string &text);
std::string joinCodes(const std::vector<std::string> &codes);
std::string kindName(PartKind kind);
std::string resultText(const AttemptResult &result);
double readNumber(const std::string &text);
} // namespace ui
} // namespace exam
#endif
