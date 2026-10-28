#ifndef SMARTEXAM_STORE_H
#define SMARTEXAM_STORE_H
#include "Database.h"
#include "core/Types.h"
namespace exam {
struct SessionRecord {
    std::string id, studentId;
    SessionSettings settings;
    std::vector<std::string> tasks;
};
struct AttemptRecord {
    std::string id, sessionId, taskId;
    int position, score;
    bool finished, provisional;
    AttemptRecord() : position(0), score(0), finished(false), provisional(true) {}
};
struct ResponseRecord {
    std::string requestId, attemptId, partId, graph, operation, operand;
    AnswerPayload answer;
    CheckResult result;
    bool finalAnswer;
    ResponseRecord() : finalAnswer(true) {}
};
struct Observation {
    std::string skill;
    bool correct, eligible;
    SkillState before, after;
};
class Store {
    Database db;
public:
    explicit Store(const std::string& path, const std::string& migrations = "db/migrations");
    void saveProfile(const std::string& id, const std::string& name);
    void saveTopic(const std::string& id, const std::string& name);
    void saveSkill(const std::string& id, const std::string& topic, const std::string& name);
    void savePrerequisite(const std::string& skill, const std::string& prerequisite);
    std::map<std::string, std::vector<std::string> > skillGraph();
    void saveDraft(const TaskVersion& task);
    void publish(const std::string& id);
    void archive(const std::string& id);
    TaskVersion task(const std::string& id);
    std::vector<TaskVersion> publishedTasks();
    void createSession(const SessionRecord& session);
    SessionRecord session(const std::string& id);
    AttemptRecord openAttempt(const std::string& sessionId, int position, const std::string& taskId);
    AttemptRecord attempt(const std::string& id);
    std::vector<AttemptRecord> attempts(const std::string& sessionId);
    bool response(const std::string& requestId, ResponseRecord& output);
    void saveResponse(const ResponseRecord& response);
    std::vector<ResponseRecord> responses(const std::string& attemptId);
    void addHelp(const std::string& requestId, const std::string& attemptId, const std::string& partId, const std::string& kind);
    bool usedHelp(const std::string& attemptId, const std::string& partId);
    void finish(const std::string& attemptId, int score, bool provisional, const std::vector<Observation>& observations);
    StudentSnapshot snapshot(const std::string& studentId);
    int observationCount(const std::string& attemptId);
};
}
#endif
