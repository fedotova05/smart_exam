#ifndef SMARTEXAM_TYPES_H
#define SMARTEXAM_TYPES_H

#include <map>
#include <string>
#include <vector>

namespace exam {
enum class CheckStatus { Correct, Incorrect, Unsupported, InvalidInput, Incomplete };
struct CheckResult {
    CheckStatus status;
    std::string message;
    int errorPosition;
    CheckResult(CheckStatus value = CheckStatus::Unsupported,
                const std::string &text = "Проверка пока недоступна", int position = -1)
        : status(value), message(text), errorPosition(position) {}
};
enum class PartKind { Expression, Equation, Line, Parabola };
struct Point {
    double x;
    double y;
    Point(double a = 0, double b = 0) : x(a), y(b) {}
};
struct GraphAnswer {
    std::vector<Point> points;
    std::vector<double> coefficients;
};
struct AnswerPayload {
    std::string text;
    GraphAnswer graph;
};
struct TaskPart {
    std::string id;
    PartKind kind;
    std::string prompt;
    std::string expression;
    std::vector<std::string> skills;
    int maxScore;
    double graphSnap, graphTolerance;
    TaskPart() : kind(PartKind::Equation), maxScore(1), graphSnap(0.5), graphTolerance(0.0001) {}
};
struct TaskVersion {
    std::string id;
    std::string title;
    std::string description;
    std::string commonFunction;
    std::vector<std::string> resources;
    std::vector<TaskPart> parts;
    bool published;
    bool archived;
    std::string generator;
    unsigned seed;
    std::string parameters;
    double difficulty;
    TaskVersion() : published(false), archived(false), seed(0), difficulty(1000) {}
};
enum class SessionMode { Fixed, Dynamic };
enum class FeedbackPolicy { AfterStep, AfterTask, AfterSet };
struct SessionSettings {
    SessionMode mode;
    FeedbackPolicy feedback;
    bool guided;
    int taskLimit;
    SessionSettings()
        : mode(SessionMode::Fixed), feedback(FeedbackPolicy::AfterStep), guided(false),
          taskLimit(10) {}
};
struct SkillState {
    bool known;
    double mastery;
    double rating;
    int observations;
    SkillState() : known(false), mastery(0.2), rating(1000), observations(0) {}
};
struct StudentSnapshot {
    std::string studentId;
    std::map<std::string, SkillState> skills;
    std::string modelVersion;
    StudentSnapshot() : modelVersion("bkt-elo-1") {}
};
struct Selection {
    bool found;
    TaskVersion task;
    std::string reason;
    Selection() : found(false) {}
};
struct PartEvidence {
    std::vector<std::string> skills;
    CheckStatus finalStatus;
    bool hadError, usedHelp;
    PartEvidence() : finalStatus(CheckStatus::Incomplete), hadError(false), usedHelp(false) {}
};
struct Observation {
    std::string skill;
    bool correct, eligible;
    SkillState before, after;
};
struct ModelParameters {
    std::string version;
    double initial, learn, slip, guess, k;
    ModelParameters()
        : version("bkt-elo-1"), initial(0.2), learn(0.1), slip(0.1), guess(0.2), k(32) {}
};
} // namespace exam
#endif
