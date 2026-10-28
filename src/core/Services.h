#ifndef SMARTEXAM_SERVICES_H
#define SMARTEXAM_SERVICES_H
#include "Types.h"
#include <stdexcept>
namespace exam {
class Checker {
public:
    virtual ~Checker() {}
    virtual CheckResult checkAnswer(const TaskPart&, const AnswerPayload&) const = 0;
    virtual CheckResult checkStep(const std::string& before, const std::string& after,
                                  const std::string& operation = "",
                                  const std::string& operand = "") const = 0;
};
class GraphChecker {
public:
    virtual ~GraphChecker() {}
    virtual CheckResult check(const TaskPart&, const GraphAnswer&) const = 0;
};
// Подключается участником 2. Отсутствие проверки не означает верный ответ.
class UnavailableGraphChecker : public GraphChecker {
public:
    CheckResult check(const TaskPart&, const GraphAnswer&) const override {
        return CheckResult(CheckStatus::Unsupported, "Графическая проверка ещё не подключена");
    }
};
}
#endif
