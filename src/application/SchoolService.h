#ifndef SMARTEXAM_SCHOOL_SERVICE_H
#define SMARTEXAM_SCHOOL_SERVICE_H
#include "session/SessionService.h"
#include "adaptation/Adaptation.h"
#include "graph_tasks/GeometryChecker.h"
#include "math/checkers/EquationChecker.h"
#include <memory>
namespace exam {
struct HistoryEntry {
    std::string attemptId, taskId, title, reason;
    AttemptResult result;
    int help;
};
class SchoolService {
    Store &storage;
    GeometryChecker geometry;
    EquationChecker checker;
    std::unique_ptr<Adaptation> adaptation;
    std::unique_ptr<SessionService> session;
    std::string activeSession;
    void prepareSession();

  public:
    explicit SchoolService(Store &store) : storage(store), checker(geometry) {}
    std::vector<std::pair<std::string, std::string>> profiles();
    std::string createProfile(const std::string &name);
    std::vector<TaskVersion> tasks();
    TaskVersion task(const std::string &id);
    std::vector<std::string> validate(const TaskVersion &task) const;
    void saveDraft(const TaskVersion &task);
    void publish(const std::string &id);
    void archive(const std::string &id);
    TaskVersion newVersion(const std::string &id);
    std::vector<SkillInfo> skills();
    std::vector<std::pair<std::string, std::string>> topics();
    std::map<std::string, std::vector<std::string>> skillGraph();
    void saveTopic(const std::string &id, const std::string &name);
    void saveSkill(const SkillInfo &skill, const std::vector<std::string> &parents);
    void placeSkill(const std::string &id, Point point);
    StudentSnapshot snapshot(const std::string &profile);
    std::vector<HistoryEntry> history(const std::string &profile);
    void start(const std::string &profile, const SessionSettings &settings,
               const std::vector<std::string> &tasks);
    bool resume(const std::string &profile);
    NextTask next();
    CheckResult submit(const std::string &attempt, const std::string &part,
                       const AnswerPayload &answer, bool finalAnswer,
                       const std::string &operation = "", const std::string &operand = "");
    std::vector<ResponseRecord> responses(const std::string &attempt);
    void help(const std::string &attempt, const std::string &part);
    AttemptResult finish(const std::string &attempt);
    void closeLesson();
    void addSampleBank();
    void addStarterContent();
};
} // namespace exam
#endif
