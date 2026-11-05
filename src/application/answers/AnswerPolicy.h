#ifndef SMARTEXAM_ANSWER_POLICY_H
#define SMARTEXAM_ANSWER_POLICY_H
#include "core/Types.h"
namespace exam {
std::string answerPolicyError(const TaskPart &part);
CheckResult checkStructuredAnswer(const TaskPart &part, const AnswerPayload &answer);
bool solvedLinearStep(const std::string &equation);
} // namespace exam
#endif
