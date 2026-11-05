#ifndef SMARTEXAM_STORE_H
#define SMARTEXAM_STORE_H
#include "Database.h"
#include "core/Types.h"
#include <functional>
namespace exam {
struct SkillInfo {
    std::string id, topicId, name;
    Point position;
    bool placed;
    SkillInfo() : placed(false) {}
};
struct SessionRecord {
    std::string id, studentId;
    SessionSettings settings;
    std::vector<std::string> tasks;
    bool closed;
    SessionRecord() : closed(false) {}
};
struct AttemptRecord {
    std::string id, sessionId, taskId, selectionReason;
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
struct TrainingStep {
    std::string origin, stage, skill;
};
struct LessonTemplate {
    std::string id, name;
    SessionSettings settings;
    std::vector<std::string> tasks;
};
struct SessionTime {
    bool allowed, paused;
    long long remainingSeconds;
    std::string message;
    SessionTime() : allowed(true), paused(false), remainingSeconds(-1) {}
};
struct ReviewVerdict {
    std::string requestId, attemptId, partId, reviewer, reason;
    CheckStatus status;
    ReviewVerdict() : status(CheckStatus::Incomplete) {}
};
class Store {
    Database db;
    std::string runtimeId, databasePath;
    struct ReplayData {
        std::map<std::string, StudentSnapshot> students;
        std::map<std::string, double> difficulties;
        std::map<std::string, std::pair<int, bool>> grades;
        std::vector<std::pair<std::string, Observation>> observations;
        Row revision;
    };
    Row modelRevision();
    ReplayData calculateReplay(const ReviewVerdict *pending = 0,
                               const std::function<void()> &checkpoint = std::function<void()>());
    void saveReplay(const ReplayData &data);

  public:
    explicit Store(const std::string &path, const std::string &migrations = "db/migrations");
    const std::string &path() const {
        return databasePath;
    }
    void saveProfile(const std::string &id, const std::string &name);
    void saveTopic(const std::string &id, const std::string &name);
    void saveSkill(const std::string &id, const std::string &topic, const std::string &name);
    void savePrerequisite(const std::string &skill, const std::string &prerequisite);
    std::map<std::string, std::vector<std::string>> skillGraph();
    void saveDraft(const TaskVersion &task);
    void publish(const std::string &id);
    void archive(const std::string &id);
    TaskVersion task(const std::string &id);
    std::vector<TaskVersion> publishedTasks();
    void createSession(const SessionRecord &session);
    SessionRecord session(const std::string &id);
    AttemptRecord openAttempt(const std::string &sessionId, int position, const std::string &taskId,
                              const std::string &reason = "", const TrainingStep *step = 0);
    void closeSession(const std::string &sessionId);
    AttemptRecord attempt(const std::string &id);
    std::vector<AttemptRecord> attempts(const std::string &sessionId);
    bool response(const std::string &requestId, ResponseRecord &output);
    void saveResponse(const ResponseRecord &response);
    std::vector<ResponseRecord> responses(const std::string &attemptId);
    void addHelp(const std::string &requestId, const std::string &attemptId,
                 const std::string &partId, const std::string &kind);
    bool usedHelp(const std::string &attemptId, const std::string &partId);
    void finish(const std::string &attemptId, int score, bool provisional,
                const std::vector<Observation> &observations, const Row &expectedRevision = Row());
    Row learningRevision() {
        return modelRevision();
    }
    StudentSnapshot snapshot(const std::string &studentId);
    StudentSnapshot attemptSnapshot(const std::string &attemptId);
    int observationCount(const std::string &attemptId);
    ModelParameters modelParameters(const std::string &version = "bkt-elo-1");
    std::vector<std::pair<std::string, std::string>> profiles();
    std::vector<std::pair<std::string, std::string>> topics();
    std::vector<SkillInfo> skills();
    std::vector<TaskVersion> allTasks();
    std::vector<SessionRecord> studentSessions(const std::string &student);
    int helpCount(const std::string &attemptId);
    void setPrerequisites(const std::string &skill, const std::vector<std::string> &parents);
    void saveSkillPosition(const std::string &skill, Point position);
    void saveSkillData(const SkillInfo &skill, const std::vector<std::string> &parents);
    std::string supportOrigin(const std::string &session);
    void beginSupport(const std::string &session, const std::string &attempt);
    void endSupport(const std::string &session);
    TrainingStep trainingStep(const std::string &attempt);
    AttemptRecord openTrainingAttempt(const std::string &session, int position,
                                      const std::string &task, const std::string &reason,
                                      const TrainingStep &step);
    void saveTemplate(const LessonTemplate &value);
    LessonTemplate lessonTemplate(const std::string &id);
    void startTemplate(const std::string &templateId, const std::string &sessionId,
                       const std::string &student);
    SessionTime sessionTime(const std::string &id, long long now = -1);
    void pauseSession(const std::string &id, long long now = -1);
    void resumeSession(const std::string &id, long long now = -1);
    void requireSessionAccess(const std::string &id);
    void saveAnswerDraft(const std::string &attempt, const std::string &part,
                         const AnswerPayload &answer);
    std::map<std::string, AnswerPayload> answerDrafts(const std::string &attempt);
    void reviewPart(const ReviewVerdict &verdict);
    std::vector<ReviewVerdict> reviewHistory(const std::string &attempt);
    void replayModels(const std::function<void()> &checkpoint = std::function<void()>());
    double taskRating(const std::string &task);
};
} // namespace exam
#endif
