#ifndef SMARTEXAM_TRAINING_SERVICE_H
#define SMARTEXAM_TRAINING_SERVICE_H
#include "application/session/SessionService.h"
namespace exam {
struct TrainingPlan {
    std::string stage, skill, reason;
    std::vector<std::string> pending;
    int successes;
    TrainingPlan() : stage("diagnosis"), successes(0) {}
};
class TrainingService {
    Store &store;
    bool successful(const AttemptRecord &attempt);
    std::vector<std::string> failedSkills(const std::string &attempt);
    std::string skillName(const std::string &skill);
    TrainingPlan plan(const std::string &session, const std::string &origin);
    std::vector<TaskVersion> exercises(const std::string &skill,
                                       const std::vector<AttemptRecord> &history);

  public:
    explicit TrainingService(Store &storage) : store(storage) {}
    std::vector<TaskVersion> scopedTasks(const std::vector<std::string> &skills);
    bool canSupport(const std::string &attempt);
    void beginSupport(const std::string &attempt);
    NextTask next(const std::string &session);
};
} // namespace exam
#endif
