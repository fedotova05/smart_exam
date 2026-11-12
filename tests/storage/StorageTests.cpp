#include "../Test.h"
#include "storage/Store.h"
#include "../Fixture.h"
#include "application/SchoolService.h"
#include <cstdio>
using namespace exam;
void testStorage() {
    const std::string path = "bin/storage-test.db";
    std::remove(path.c_str());
    {
        Store store(path);
        store.saveProfile("student", "Тестовая ученица");
        store.saveTopic("algebra", "Алгебра");
        store.saveSkill("linear", "algebra", "Линейное уравнение");
        TaskVersion task;
        task.id = "v1";
        task.title = "Пример";
        TaskPart part;
        part.id = "a";
        part.prompt = "Решите";
        part.expression = "x+1=2";
        part.skills.push_back("linear");
        task.parts.push_back(part);
        store.saveDraft(task);
        store.publish(task.id);
        bool rejected = false;
        try {
            task.parts[0].expression = "x=99";
            store.saveDraft(task);
        } catch (const std::exception &) {
            rejected = true;
        }
        REQUIRE(rejected);
        REQUIRE(store.task("v1").parts[0].expression == "x+1=2");
        task.id = "broken";
        task.parts[0].skills[0] = "missing";
        rejected = false;
        try {
            store.saveDraft(task);
        } catch (const std::exception &) {
            rejected = true;
        }
        REQUIRE(rejected);
        try {
            store.task("broken");
            REQUIRE(false);
        } catch (const std::runtime_error &e) {
            REQUIRE(std::string(e.what()) == "Task not found");
        }
        SessionRecord session;
        session.id = "s1";
        session.studentId = "student";
        session.tasks.push_back("v1");
        store.createSession(session);
        AttemptRecord attempt = store.openAttempt("s1", 0, "v1");
        ResponseRecord response;
        response.requestId = "r1";
        response.attemptId = attempt.id;
        response.partId = "a";
        response.answer.text = "1";
        store.saveResponse(response);
        store.saveResponse(response);
        REQUIRE(store.responses(attempt.id).size() == 1);
        store.finish(attempt.id, 0, true, {});
        store.finish(attempt.id, 100, false, {});
        REQUIRE(store.attempt(attempt.id).score == 0);
        store.archive("v1");
        REQUIRE(store.publishedTasks().empty());
        REQUIRE(store.task(store.attempt(attempt.id).taskId).parts[0].expression == "x+1=2");
        Database legacy(path);
        legacy.script("DROP TABLE exam_tasks; DROP TABLE lecture_skills; DROP TABLE lectures; "
                      "PRAGMA user_version=13;");
    }
    {
        Store reopened(path);
        REQUIRE(reopened.attempt("s1:0").finished);
        REQUIRE(reopened.responses("s1:0").size() == 1);
        Database audit(path);
        REQUIRE(audit.query("PRAGMA user_version")[0][0] == "14");
        REQUIRE(reopened.lectures().empty());
        addTask(reopened, "failure-test");
        SchoolService school(reopened);
        school.start("student", SessionSettings(), {"failure-test"});
        auto current = school.next();
        AnswerPayload answer;
        answer.text = "1";
        REQUIRE(school.submit(current.attempt.id, "answer", answer, true).status ==
                CheckStatus::Correct);
        for (auto table : {"observations", "student_skills"}) {
            audit.script(std::string("CREATE TRIGGER injected_failure BEFORE INSERT ON ") + table +
                         " BEGIN SELECT RAISE(ABORT,'write rejected'); END;");
            bool failed = false;
            try {
                school.finish(current.attempt.id);
            } catch (const std::exception &) {
                failed = true;
            }
            REQUIRE(failed);
            REQUIRE(!reopened.attempt(current.attempt.id).finished);
            REQUIRE(reopened.responses(current.attempt.id).size() == 1);
            REQUIRE(reopened.observationCount(current.attempt.id) == 0);
            REQUIRE(!reopened.snapshot("student").skills["linear"].known);
            audit.script("DROP TRIGGER injected_failure;");
        }
        school.finish(current.attempt.id);
        school.finish(current.attempt.id);
        REQUIRE(reopened.observationCount(current.attempt.id) == 1);
        auto before = reopened.snapshot("student").skills["linear"];
        reopened.replayModels();
        auto after = reopened.snapshot("student").skills["linear"];
        REQUIRE(before.mastery == after.mastery && before.rating == after.rating);
    }
    std::remove(path.c_str());
}
