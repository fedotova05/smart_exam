#include "SessionService.h"
#include "core/GraphCodec.h"
#include <stdexcept>
namespace exam {
void SessionService::start(const SessionRecord& session) {
    if(session.settings.mode==SessionMode::Dynamic && !selector) throw std::runtime_error("Динамический подбор ещё не подключён");
    store.createSession(session);
}
NextTask SessionService::next(const std::string& id) {
    SessionRecord session=store.session(id);
    std::vector<AttemptRecord> attempts=store.attempts(id);
    NextTask result;
    if(!attempts.empty() && !attempts.back().finished) {
        result.available=true; result.attempt=attempts.back(); result.task=store.task(result.attempt.taskId); result.reason="Продолжение текущего задания"; return result;
    }
    if(attempts.size()>=static_cast<std::size_t>(session.settings.taskLimit) || (session.settings.mode==SessionMode::Fixed && attempts.size()>=session.tasks.size())) {
        result.reason="Занятие завершено"; return result;
    }
    std::string taskId;
    if(session.settings.mode==SessionMode::Fixed) { taskId=session.tasks[attempts.size()]; result.reason="Следующее задание фиксированного набора"; }
    else {
        if(!selector) { result.reason="Динамический подбор ещё не подключён"; return result; }
        std::vector<std::string> history;
        for(const AttemptRecord& attempt:attempts) history.push_back(attempt.taskId);
        Selection selected=selector->selectNext(store.snapshot(session.studentId),store.publishedTasks(),history);
        result.reason=selected.reason;
        if(!selected.found) return result;
        taskId=selected.task.id;
    }
    result.attempt=store.openAttempt(id,static_cast<int>(attempts.size()),taskId);
    result.task=store.task(taskId); result.available=true; return result;
}
bool SessionService::feedbackVisible(const AttemptRecord& attempt) {
    SessionRecord session=store.session(attempt.sessionId);
    if(session.settings.feedback==FeedbackPolicy::AfterStep) return true;
    if(session.settings.feedback==FeedbackPolicy::AfterTask) return attempt.finished;
    std::vector<AttemptRecord> attempts=store.attempts(session.id);
    int expected=session.settings.mode==SessionMode::Fixed?static_cast<int>(session.tasks.size()):session.settings.taskLimit;
    if(static_cast<int>(attempts.size())<expected) return false;
    for(const AttemptRecord& value:attempts) if(!value.finished) return false;
    return true;
}
CheckResult SessionService::submitResponse(const ResponseRecord& input) {
    ResponseRecord response=input;
    response.graph=encodeGraph(input.answer.graph);
    AttemptRecord attempt=store.attempt(response.attemptId);
    ResponseRecord saved;
    if(store.response(response.requestId,saved)) {
        store.saveResponse(response);
        return feedbackVisible(attempt)?saved.result:CheckResult(CheckStatus::Incomplete,"Ответ сохранён; проверка будет показана позже");
    }
    if(attempt.finished) throw std::runtime_error("Попытка уже завершена");
    if(response.answer.text.size()>512) throw std::invalid_argument("Ответ длиннее 512 байтов");
    TaskVersion task=store.task(attempt.taskId);
    const TaskPart* part=0;
    for(const TaskPart& value:task.parts) if(value.id==response.partId) part=&value;
    if(!part) throw std::runtime_error("Часть задания не найдена");
    if(response.finalAnswer) response.result=checker.checkAnswer(*part,response.answer);
    else if(part->kind!=PartKind::Equation) response.result=CheckResult(CheckStatus::Unsupported,"Шаги поддерживаются для уравнений");
    else {
        std::string before=part->expression;
        for(const ResponseRecord& old:store.responses(attempt.id)) {
            if(old.partId==part->id && !old.finalAnswer && old.result.status==CheckStatus::Correct) before=old.answer.text;
        }
        SessionRecord session=store.session(attempt.sessionId);
        if(session.settings.guided && response.operation.empty()) response.result=CheckResult(CheckStatus::Incomplete,"Выберите действие над уравнением");
        else response.result=checker.checkStep(before,response.answer.text,session.settings.guided?response.operation:"",session.settings.guided?response.operand:"");
    }
    store.saveResponse(response);
    return feedbackVisible(attempt)?response.result:CheckResult(CheckStatus::Incomplete,"Ответ сохранён; проверка будет показана позже");
}
void SessionService::requestHelp(const std::string& request,const std::string& attempt,const std::string& part,const std::string& kind) { store.addHelp(request,attempt,part,kind); }
AttemptResult SessionService::finishAttempt(const std::string& id) {
    AttemptRecord attempt=store.attempt(id);
    if(attempt.finished) return result(id);
    TaskVersion task=store.task(attempt.taskId);
    std::vector<ResponseRecord> responses=store.responses(id);
    std::vector<PartEvidence> evidence;
    int score=0; bool provisional=false;
    for(const TaskPart& part:task.parts) {
        PartEvidence item; item.skills=part.skills; item.usedHelp=store.usedHelp(id,part.id);
        for(const ResponseRecord& response:responses) {
            if(response.partId!=part.id) continue;
            if(response.finalAnswer) item.finalStatus=response.result.status;
            if(response.result.status==CheckStatus::Incorrect) item.hadError=true;
        }
        if(item.finalStatus==CheckStatus::Correct) score+=part.maxScore;
        else if(item.finalStatus!=CheckStatus::Incorrect) provisional=true;
        evidence.push_back(item);
    }
    std::vector<Observation> observations;
    if(learning) observations=learning->observe(store.attemptSnapshot(id),evidence,task.difficulty);
    store.finish(id,score,provisional,observations);
    return result(id);
}
AttemptResult SessionService::result(const std::string& id) {
    AttemptRecord attempt=store.attempt(id);
    AttemptResult output; output.finished=attempt.finished; output.visible=attempt.finished && feedbackVisible(attempt);
    if(output.visible) {
        output.score=attempt.score; output.provisional=attempt.provisional;
        output.message=attempt.provisional?"Предварительный результат: есть непроверенные части":"Результат сохранён";
    } else output.message=attempt.finished?"Результат сохранён; оценка пока скрыта":"Попытка ещё не завершена";
    return output;
}
std::vector<ResponseRecord> SessionService::history(const std::string& id) {
    bool visible=feedbackVisible(store.attempt(id));
    std::vector<ResponseRecord> result=store.responses(id);
    if(!visible) for(ResponseRecord& response:result) response.result=CheckResult(CheckStatus::Incomplete,"Проверка пока скрыта");
    return result;
}
}
