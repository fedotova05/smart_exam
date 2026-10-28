#ifndef SMARTEXAM_SESSION_SERVICE_H
#define SMARTEXAM_SESSION_SERVICE_H
#include "core/Services.h"
#include "storage/Store.h"
namespace exam {
struct NextTask {
    bool available;
    std::string reason;
    AttemptRecord attempt;
    TaskVersion task;
    NextTask() : available(false) {}
};
struct AttemptResult {
    bool finished, visible, provisional;
    int score;
    std::string message;
    AttemptResult() : finished(false),visible(false),provisional(true),score(0) {}
};
class SessionService {
    Store& store;
    const Checker& checker;
    const TaskSelector* selector;
    const LearningModel* learning;
    bool feedbackVisible(const AttemptRecord& attempt);
public:
    SessionService(Store& storage,const Checker& checks,
        const TaskSelector* selection = 0,const LearningModel* model = 0)
        : store(storage),checker(checks),selector(selection),learning(model) {}
    void start(const SessionRecord& session);
    NextTask next(const std::string& sessionId);
    CheckResult submitResponse(const ResponseRecord& response);
    void requestHelp(const std::string& requestId,const std::string& attemptId,
        const std::string& partId,const std::string& kind);
    AttemptResult finishAttempt(const std::string& attemptId);
    AttemptResult result(const std::string& attemptId);
    std::vector<ResponseRecord> history(const std::string& attemptId);
};
}
#endif
