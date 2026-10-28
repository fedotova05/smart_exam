#ifndef SMARTEXAM_GENERATOR_H
#define SMARTEXAM_GENERATOR_H
#include "core/Types.h"
#include "storage/Store.h"
#include <set>
namespace exam {
enum class Family { Expression, Linear, Quadratic, Line, Parabola };
struct GenerationRequest {
    std::string id;
    Family family;
    unsigned seed;
    int maxAttempts;
    std::set<std::string> previousExpressions;
    GenerationRequest() : family(Family::Linear),seed(1),maxAttempts(32) {}
};
class Generator {
public:
    TaskVersion generateTask(const GenerationRequest& request) const;
    TaskVersion generateAndSave(Store& store,const GenerationRequest& request) const;
};
}
#endif
