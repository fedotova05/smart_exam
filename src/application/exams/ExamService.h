#ifndef SMARTEXAM_EXAM_SERVICE_H
#define SMARTEXAM_EXAM_SERVICE_H
#include "application/SchoolService.h"
#include <functional>
namespace exam {
struct ExamCategory {
    int number, id;
    std::string name, skill;
};
struct ImportedExamTask {
    ExamTask metadata;
    TaskVersion task;
    std::vector<std::string> images;
};
struct ExamLoadResult {
    std::vector<std::string> tasks;
    std::vector<std::string> warnings;
    int added = 0;
};
const std::vector<ExamCategory> &ogeCategories();
void installOgeCurriculum(Store &store);
std::vector<ImportedExamTask> parseSdamgia(const std::string &html, const ExamCategory &category);
class ExamService {
    Store &store;

  public:
    explicit ExamService(Store &s) : store(s) {}
    ExamLoadResult load(int category, int count, bool online,
                        const std::function<void()> &checkpoint = {},
                        const std::vector<std::string> &excluded = {});
    ExamLoadResult importPackage(const std::string &path,
                                 const std::function<void()> &checkpoint = {});
    std::string importTask(ImportedExamTask value,
                           const std::function<std::string(const std::string &)> &download);
};
} // namespace exam
#endif
