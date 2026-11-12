#include "../Test.h"
#include "storage/Store.h"
#include "application/session/SessionService.h"
#include "math/checkers/EquationChecker.h"
#include <ctime>
#include "io/Files.h"
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
    result.source = "teacher";
    result.title = "Сохранённое назначение";
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
        Database old(path);
        const char *migrations[] = {
            "001_initial.sql",          "002_attempt_snapshot.sql", "003_session_completion.sql",
            "004_interface_data.sql",   "005_graph_settings.sql",   "006_training.sql",
            "007_session_policies.sql", "008_manual_review.sql",    "009_answer_policies.sql",
            "010_teacher_access.sql",   "011_lesson_options.sql",   "012_lesson_source.sql"};
        for (const auto &file : migrations)
            old.script(readFile(std::string("db/migrations/") + file));
        REQUIRE(old.query("PRAGMA user_version")[0][0] == "12");
        old.query("INSERT INTO "
                  "tasks(id,title,published,archived,generator,seed,parameters,difficulty,"
                  "description,common_function) VALUES('old','Старое',0,0,'',0,'',1000,'','')");
        old.query("INSERT INTO parts VALUES('old','answer',1,'Решите','x=2',1,0,0.5,0.0001,'')");
        old.query("INSERT INTO part_policies VALUES('old','answer',0,-1,0)");
    }
    {
        Store store(path);
        REQUIRE(store.task("old").parts[0].correctAnswer.empty());
        REQUIRE(store.task("old").parts[0].expression == "x=2");
        seed(store);
        TaskVersion shortTask = store.task("task");
        shortTask.id = "short";
        shortTask.published = false;
        shortTask.parts[0].kind = PartKind::ShortAnswer;
        shortTask.parts[0].expression.clear();
        shortTask.parts[0].correctAnswer = "01342";
        store.saveDraft(shortTask);
        store.publish(shortTask.id);
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
        REQUIRE(store.task("short").parts[0].correctAnswer == "01342");
        REQUIRE(store.task("short").parts[0].kind == PartKind::ShortAnswer);
        REQUIRE(store.session("active").source == "teacher");
        REQUIRE(store.session("active").title == "Сохранённое назначение");
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
