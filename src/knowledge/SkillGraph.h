#ifndef SMARTEXAM_SKILL_GRAPH_H
#define SMARTEXAM_SKILL_GRAPH_H
#include "core/Types.h"
namespace exam {
class SkillGraph {
    std::map<std::string, std::vector<std::string>> prerequisites;

  public:
    explicit SkillGraph(const std::map<std::string, std::vector<std::string>> &values);
    bool contains(const std::string &skill) const;
    int missingPrerequisites(const std::string &skill, const StudentSnapshot &student) const;
};
} // namespace exam
#endif
