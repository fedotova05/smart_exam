#include "SessionService.h"
#include "core/GraphCodec.h"
#include "math/checkers/EquationChecker.h"
#include "application/training/TrainingService.h"
#include "application/answers/AnswerPolicy.h"
#include <stdexcept>
#include <chrono>
namespace exam {
void SessionService::start(const SessionRecord &session) {
    if (session.settings.training && (session.settings.mode != SessionMode::Dynamic ||
                                      session.settings.feedback != FeedbackPolicy::AfterStep ||
                                      session.settings.targetSkills.empty()))
        throw std::invalid_argument(
            "Тренировка требует выбранных навыков, подбора заданий и немедленной проверки");
    if (session.settings.mode == SessionMode::Dynamic && !selector)
        throw std::runtime_error("Динамический подбор ещё не подключён");
    store.createSession(session);
}
NextTask SessionService::next(const std::string &id) {
    SessionRecord session = store.session(id);
    SessionTime time = store.sessionTime(id);
    if (!time.allowed) {
        NextTask stopped;
        stopped.reason = time.message;
        if (!session.closed && !time.paused) {
            for (const auto &value : store.attempts(id))
                if (!value.finished)
                    finishAttempt(value.id, true);
            store.closeSession(id);
        }
        return stopped;
    }
    if (session.settings.training)
        return TrainingService(store).next(id);
    std::vector<AttemptRecord> attempts = store.attempts(id);
    NextTask result;
    if (session.closed) {
        result.reason = "Занятие завершено";
        return result;
    }
    if (!attempts.empty() && !attempts.back().finished) {
        result.available = true;
        result.attempt = attempts.back();
        result.task = store.task(result.attempt.taskId);
        result.reason = "Продолжение текущего задания";
        return result;
    }
    if (attempts.size() >= static_cast<std::size_t>(session.settings.taskLimit) ||
        (session.settings.mode == SessionMode::Fixed && attempts.size() >= session.tasks.size())) {
        store.closeSession(id);
        result.reason = "Занятие завершено";
        return result;
    }
    std::string taskId;
    if (session.settings.mode == SessionMode::Fixed) {
        taskId = session.tasks[attempts.size()];
        result.reason = "Следующее задание фиксированного набора";
    } else {
        if (!selector) {
            result.reason = "Динамический подбор ещё не подключён";
            return result;
        }
        std::vector<std::string> history;
        for (const AttemptRecord &attempt : attempts)
            history.push_back(attempt.taskId);
        Selection selected = selector->selectNext(store.snapshot(session.studentId),
                                                  store.publishedTasks(), history);
        result.reason = selected.reason;
        if (!selected.found) {
            store.closeSession(id);
            return result;
        }
        taskId = selected.task.id;
    }
    result.attempt =
        store.openAttempt(id, static_cast<int>(attempts.size()), taskId, result.reason);
    result.task = store.task(taskId);
    result.available = true;
    return result;
}
bool SessionService::feedbackVisible(const AttemptRecord &attempt) {
    SessionRecord session = store.session(attempt.sessionId);
    if (session.closed)
        return true;
    if (session.settings.feedback == FeedbackPolicy::AfterStep)
        return true;
    if (session.settings.feedback == FeedbackPolicy::AfterTask)
        return attempt.finished;
    std::vector<AttemptRecord> attempts = store.attempts(session.id);
    int expected = session.settings.mode == SessionMode::Fixed
                       ? static_cast<int>(session.tasks.size())
                       : session.settings.taskLimit;
    if (static_cast<int>(attempts.size()) < expected)
        return false;
    for (const AttemptRecord &value : attempts)
        if (!value.finished)
            return false;
    return true;
}
CheckResult SessionService::submitResponse(const ResponseRecord &input) {
    ResponseRecord response = input;
    response.graph = encodeGraph(input.answer.graph);
    AttemptRecord attempt = store.attempt(response.attemptId);
    ResponseRecord saved;
    if (store.response(response.requestId, saved)) {
        store.saveResponse(response);
        return feedbackVisible(attempt)
                   ? saved.result
                   : CheckResult(CheckStatus::Incomplete,
                                 "Ответ сохранён; проверка будет показана позже");
    }
    if (attempt.finished)
        throw std::runtime_error("Попытка уже завершена");
    store.requireSessionAccess(attempt.sessionId);
    if (response.answer.text.size() > 512)
        throw std::invalid_argument("Ответ длиннее 512 байтов");
    TaskVersion task = store.task(attempt.taskId);
    const TaskPart *part = 0;
    for (const TaskPart &value : task.parts)
        if (value.id == response.partId)
            part = &value;
    if (!part)
        throw std::runtime_error("Часть задания не найдена");
    if (response.operation.size() > 32 || response.operand.size() > 100)
        throw std::invalid_argument("Слишком длинная запись действия");
    auto checkStarted = std::chrono::steady_clock::now();
    if (response.finalAnswer) {
        response.result = checker.checkAnswer(*part, response.answer);
        if (part->requireSteps && response.result.status == CheckStatus::Correct) {
            bool solved = false;
            for (const auto &old : store.responses(attempt.id))
                if (old.partId == part->id && !old.finalAnswer &&
                    old.result.status == CheckStatus::Correct && !old.operation.empty() &&
                    solvedLinearStep(old.answer.text))
                    solved = true;
            if (!solved)
                response.result = CheckResult(CheckStatus::Incomplete,
                                              "Ответ верный, но требуется решение: выполните "
                                              "преобразования до x = число и отправьте итог снова");
        }
    } else if (part->kind != PartKind::Equation || !isLinearEquation(part->expression))
        response.result =
            CheckResult(CheckStatus::Unsupported,
                        "Шаги доступны только для линейных уравнений. Введите итоговый ответ.");
    else {
        std::string before = part->expression;
        for (const ResponseRecord &old : store.responses(attempt.id)) {
            if (old.partId == part->id && !old.finalAnswer &&
                old.result.status == CheckStatus::Correct)
                before = old.answer.text;
        }
        response.result =
            checker.checkStep(before, response.answer.text, response.operation, response.operand);
    }
    if (std::chrono::steady_clock::now() - checkStarted > std::chrono::milliseconds(200))
        response.result =
            CheckResult(CheckStatus::Unsupported, "Проверка превысила отведённое время");
    store.saveResponse(response);
    return feedbackVisible(attempt) ? response.result
                                    : CheckResult(CheckStatus::Incomplete,
                                                  "Ответ сохранён; проверка будет показана позже");
}
void SessionService::requestHelp(const std::string &request, const std::string &attempt,
                                 const std::string &part, const std::string &kind) {
    store.requireSessionAccess(store.attempt(attempt).sessionId);
    store.addHelp(request, attempt, part, kind);
}
AttemptResult SessionService::finishAttempt(const std::string &id, bool timeExpired) {
    AttemptRecord attempt = store.attempt(id);
    if (attempt.finished)
        return result(id);
    TaskVersion task = store.task(attempt.taskId);
    std::vector<ResponseRecord> responses = store.responses(id);
    std::vector<PartEvidence> evidence;
    int score = 0;
    bool provisional = false;
    for (const TaskPart &part : task.parts) {
        PartEvidence item;
        item.skills = part.skills;
        item.usedHelp = store.usedHelp(id, part.id);
        for (const ResponseRecord &response : responses) {
            if (response.partId != part.id)
                continue;
            if (response.finalAnswer)
                item.finalStatus = response.result.status;
            if (response.result.status == CheckStatus::Incorrect)
                item.hadError = true;
        }
        if (part.requireSteps && item.finalStatus == CheckStatus::Incomplete && !timeExpired)
            throw std::runtime_error("В части «" + part.id +
                                     "» требуется решение и отправленный итоговый ответ");
        if (item.finalStatus == CheckStatus::Correct)
            score += part.maxScore;
        else if (item.finalStatus != CheckStatus::Incorrect)
            provisional = true;
        evidence.push_back(item);
    }
    std::vector<Observation> observations;
    Row revision = store.learningRevision();
    // Возврат к разобранному условию приносит баллы, но не новое независимое наблюдение.
    if (learning && store.trainingStep(id).stage != "return")
        observations = learning->observe(store.snapshot(store.session(attempt.sessionId).studentId),
                                         evidence, store.taskRating(task.id));
    store.finish(id, score, provisional, observations, revision);
    return result(id);
}
AttemptResult SessionService::result(const std::string &id) {
    AttemptRecord attempt = store.attempt(id);
    AttemptResult output;
    output.finished = attempt.finished;
    output.visible = attempt.finished && feedbackVisible(attempt);
    if (output.visible) {
        output.score = attempt.score;
        output.provisional = attempt.provisional;
        output.message = attempt.provisional ? "Предварительный результат: есть непроверенные части"
                                             : "Результат сохранён";
    } else
        output.message = attempt.finished ? "Результат сохранён; оценка пока скрыта"
                                          : "Попытка ещё не завершена";
    return output;
}
std::vector<ResponseRecord> SessionService::history(const std::string &id) {
    bool visible = feedbackVisible(store.attempt(id));
    std::vector<ResponseRecord> result = store.responses(id);
    if (!visible)
        for (ResponseRecord &response : result)
            response.result = CheckResult(CheckStatus::Incomplete, "Проверка пока скрыта");
    return result;
}
} // namespace exam
