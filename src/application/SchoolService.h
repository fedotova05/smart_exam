#ifndef SMARTEXAM_SCHOOL_SERVICE_H
#define SMARTEXAM_SCHOOL_SERVICE_H
#include "session/SessionService.h"
#include "adaptation/Adaptation.h"
#include "graph_tasks/GeometryChecker.h"
#include "math/checkers/EquationChecker.h"
#include <memory>
namespace exam {
struct HistoryEntry {
    std::string attemptId, taskId, title, reason, lessonId;
    long long completion = 0;
    AttemptResult result;
    int help, maximumScore;
    bool hadErrors;
    HistoryEntry() : help(0), maximumScore(0), hadErrors(false) {}
};
struct LessonProgress {
    SessionRecord lesson;
    int totalTasks, finishedTasks, score, maximumScore;
    bool hidden, provisional;
    LessonProgress()
        : totalTasks(0), finishedTasks(0), score(0), maximumScore(0), hidden(false),
          provisional(false) {}
};
struct ReviewItem {
    std::string student, studentName, lesson, attempt, task, title, part;
    CheckStatus automatic, effective;
    bool reviewed;
};
class SchoolService {
    Store &storage;
    GeometryChecker geometry;
    EquationChecker checker;
    std::unique_ptr<Adaptation> adaptation;
    std::unique_ptr<SessionService> session;
    std::string activeSession;
    void prepareSession();
    void pauseCurrent();
    void addTrainingContent();
    void addTrainingDemo();

  public:
    std::vector<Lecture> lectures(bool drafts = false);
    Lecture lecture(const std::string &id, bool drafts = false);
    void saveLecture(const Lecture &value, const std::string &file = "");
    std::vector<Lecture> recommendedLectures(const std::string &attempt);
    void readLecture(const std::string &id, const std::string &attempt = "");
    void openLectureFile(const std::string &id, bool drafts = false);
    std::vector<ExamTask> examTasks(int number = 0, int category = 0) {
        return storage.examTasks(number, category);
    }
    explicit SchoolService(Store &store) : storage(store), checker(geometry) {}
    bool hasTeacherPassword() {
        return storage.hasTeacherPassword();
    }
    void setTeacherPassword(const std::string &password) {
        storage.setTeacherPassword(password);
    }
    bool loginTeacher(const std::string &password) {
        return storage.loginTeacher(password);
    }
    void logoutTeacher() {
        storage.logoutTeacher();
    }
    void requireTeacher() const {
        storage.requireTeacher();
    }
    std::shared_ptr<TeacherAccess> teacherSession() const {
        return storage.teacherSession();
    }
    void archiveProfile(const std::string &id, bool archived) {
        storage.archiveProfile(id, archived);
    }
    std::vector<std::pair<std::string, std::string>> archivedProfiles() {
        return storage.archivedProfiles();
    }
    const std::string &databasePath() const {
        return storage.path();
    }
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
    bool resultsHidden(const std::string &profile);
    std::vector<HistoryEntry> history(const std::string &profile);
    std::vector<ReviewItem> reviewItems(const std::string &student = "", bool pendingOnly = true);
    std::vector<ReviewVerdict> reviewHistory(const std::string &attempt);
    std::vector<ResponseRecord> reviewResponses(const std::string &attempt);
    void revise(const ReviewVerdict &verdict);
    std::string reportCsv(const std::string &student, bool pendingOnly);
    SessionRecord latestLesson(const std::string &profile);
    SessionRecord currentLesson(const std::string &profile);
    std::vector<SessionRecord> lessons(const std::string &profile) {
        return storage.studentSessions(profile);
    }
    std::vector<AttemptRecord> lessonAttempts(const std::string &lesson);
    LessonProgress progress(const std::string &profile, const std::string &lessonId = "");
    void start(const std::string &profile, const SessionSettings &settings,
               const std::vector<std::string> &tasks, const std::string &source = "legacy",
               const std::string &title = "");
    bool resume(const std::string &profile, const std::string &lessonId = "");
    NextTask next();
    CheckResult submit(const std::string &attempt, const std::string &part,
                       const AnswerPayload &answer, bool finalAnswer,
                       const std::string &operation = "", const std::string &operand = "");
    std::vector<ResponseRecord> responses(const std::string &attempt);
    void help(const std::string &attempt, const std::string &part);
    AttemptResult finish(const std::string &attempt);
    void closeLesson();
    SessionTime lessonTime() {
        return storage.sessionTime(activeSession);
    }
    void pauseLesson() {
        storage.pauseSession(activeSession);
    }
    void continueLesson() {
        storage.resumeSession(activeSession);
    }
    std::vector<LessonTemplate> lessonTemplates() {
        storage.requireTeacher();
        return storage.lessonTemplates();
    }
    void saveTemplate(const LessonTemplate &value) {
        storage.requireTeacher();
        storage.saveTemplate(value);
    }
    void saveAnswerDraft(const std::string &attempt, const std::string &part,
                         const AnswerPayload &answer) {
        storage.saveAnswerDraft(attempt, part, answer);
    }
    std::map<std::string, AnswerPayload> answerDrafts(const std::string &attempt) {
        return storage.answerDrafts(attempt);
    }
    void addSampleBank();
    void addStarterContent(bool includeHistory = true);
    void addDemonstrationCourse();
    void startTopics(const std::string &profile, const std::vector<std::string> &skills,
                     bool training, int count);
    bool canSupport(const std::string &attempt);
    NextTask startSupport(const std::string &attempt);
    std::vector<std::string> reviewSkills(const std::string &profile,
                                          const std::string &lessonId = "");
};
} // namespace exam
#endif
