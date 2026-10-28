#ifndef SMARTEXAM_ADAPTATION_H
#define SMARTEXAM_ADAPTATION_H
#include "core/Services.h"
#include "knowledge/SkillGraph.h"
namespace exam {
class Adaptation : public LearningModel, public TaskSelector {
    ModelParameters parameters;
    SkillGraph graph;

  public:
    Adaptation(const ModelParameters &settings, const SkillGraph &skills);
    SkillState update(const SkillState &before, bool correct, double difficulty) const;
    std::vector<Observation> observe(const StudentSnapshot &before,
                                     const std::vector<PartEvidence> &evidence,
                                     double difficulty) const override;
    Selection selectNext(const StudentSnapshot &student, const std::vector<TaskVersion> &candidates,
                         const std::vector<std::string> &history) const override;
};
} // namespace exam
#endif
