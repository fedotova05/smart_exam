#include "SkillGraph.h"
#include <set>
#include <stdexcept>
namespace exam {
SkillGraph::SkillGraph(const std::map<std::string, std::vector<std::string>> &values)
    : prerequisites(values) {
    std::map<std::string, int> incoming;
    std::map<std::string, std::vector<std::string>> children;
    for (const auto &entry : values) {
        incoming[entry.first] = static_cast<int>(entry.second.size());
        std::set<std::string> unique;
        for (const std::string &parent : entry.second) {
            if (!values.count(parent))
                throw std::invalid_argument("В графе указан неизвестный навык");
            if (!unique.insert(parent).second)
                throw std::invalid_argument("Повтор зависимости навыка");
            children[parent].push_back(entry.first);
        }
    }
    std::vector<std::string> ready;
    for (const auto &entry : incoming)
        if (entry.second == 0)
            ready.push_back(entry.first);
    for (std::size_t i = 0; i < ready.size(); ++i)
        for (const std::string &child : children[ready[i]])
            if (--incoming[child] == 0)
                ready.push_back(child);
    if (ready.size() != values.size())
        throw std::invalid_argument("В графе навыков есть цикл");
}
bool SkillGraph::contains(const std::string &skill) const {
    return prerequisites.count(skill) != 0;
}
int SkillGraph::missingPrerequisites(const std::string &skill,
                                     const StudentSnapshot &student) const {
    auto found = prerequisites.find(skill);
    if (found == prerequisites.end())
        throw std::invalid_argument("Неизвестный навык");
    int missing = 0;
    for (const std::string &parent : found->second) {
        auto state = student.skills.find(parent);
        if (state == student.skills.end() || !state->second.known || state->second.mastery < 0.6)
            ++missing;
    }
    return missing;
}
} // namespace exam
