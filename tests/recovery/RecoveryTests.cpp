#include "../Test.h"
#include "storage/Store.h"
#include "application/session/SessionService.h"
#include "math/checkers/EquationChecker.h"
#include <ctime>
#include <cstdio>
using namespace exam;
namespace {
void seed(Store &store) {
    store.saveProfile("student", "Ученик");
    store.saveTopic("topic", "Тема");
    store.saveSkill("skill", "topic", "Навык");
    TaskVersion task;
    task.id = "task";
    task.title = "Уравнение";
    TaskPart part;
    part.id = "part";
    part.prompt = "Решите";
    part.expression = "x=2";
    part.skills = {"skill"};
    task.parts.push_back(part);
    store.saveDraft(task);
    store.publish(task.id);
}
SessionRecord lesson(const std::string &id, ResumePolicy policy) {
    SessionRecord result;
    result.id = id;
    result.studentId = "student";
    result.tasks = {"task"};
    result.settings.resumePolicy = policy;
    result.settings.timeLimitSeconds = 60;
    return result;
}
} // namespace
void testRecovery() {
    {
        Store store(":memory:");
        seed(store);
        UnavailableGraphChecker graph;
        EquationChecker checker(graph);
        SessionService service(store, checker);
        SessionRecord value = lesson("expired", ResumePolicy::Deadline);
        service.start(value);
        NextTask current = service.next(value.id);
        ResponseRecord answer;
        answer.requestId = "before-deadline";
        answer.attemptId = current.attempt.id;
        answer.partId = "part";
        answer.answer.text = "2";
        REQUIRE(service.submitResponse(answer).status == CheckStatus::Correct);
        REQUIRE(!store.sessionTime(value.id, static_cast<long long>(std::time(0)) + 61).allowed);
        answer.requestId = "after-deadline";
        bool rejected = false;
        try {
            service.submitResponse(answer);
        } catch (const std::exception &) {
            rejected = true;
        }
        REQUIRE(rejected);
        REQUIRE(!service.next(value.id).available);
        REQUIRE(store.session(value.id).closed);
        REQUIRE(store.attempt(current.attempt.id).finished);
        REQUIRE(store.attempt(current.attempt.id).score == 1);
        REQUIRE(store.responses(current.attempt.id).size() == 1);
    }
    const char *path = "bin/recovery-test.db";
    std::remove(path);
    {
        Store store(path);
        seed(store);
        LessonTemplate value;
        value.id = "template";
        value.name = "Вариант";
        value.tasks = {"task"};
        value.settings.timeLimitSeconds = 60;
        store.saveTemplate(value);
        store.startTemplate(value.id, "copy", "student");
        value.settings.timeLimitSeconds = 120;
        store.saveTemplate(value);
        REQUIRE(store.session("copy").settings.timeLimitSeconds == 60);
        auto attempt = store.openAttempt("copy", 0, "task");
        AnswerPayload draft;
        draft.text = "2/";
        store.saveAnswerDraft(attempt.id, "part", draft);
        REQUIRE(store.responses(attempt.id).empty());
        store.createSession(lesson("deadline", ResumePolicy::Deadline));
        REQUIRE(store.sessionTime("deadline", 100).remainingSeconds == 60);
        store.createSession(lesson("active", ResumePolicy::ActiveTime));
        store.sessionTime("active", 100);
        store.pauseSession("active", 120);
        store.createSession(lesson("blocked", ResumePolicy::NoResume));
        store.sessionTime("blocked", 100);
        store.createSession(lesson("rollback", ResumePolicy::Deadline));
        store.sessionTime("rollback", 100);
        REQUIRE(!store.sessionTime("rollback", 99).allowed);
    }
    {
        Store store(path);
        REQUIRE(store.answerDrafts("copy:0")["part"].text == "2/");
        REQUIRE(store.sessionTime("deadline", 150).remainingSeconds == 10);
        REQUIRE(!store.sessionTime("deadline", 161).allowed);
        REQUIRE(!store.sessionTime("blocked", 110).allowed);
        REQUIRE(store.sessionTime("active", 1000).paused);
        store.resumeSession("active", 1000);
        REQUIRE(store.sessionTime("active", 1010).remainingSeconds == 30);
        REQUIRE(!store.sessionTime("rollback", 200).allowed);
    }
    std::remove(path);
}
