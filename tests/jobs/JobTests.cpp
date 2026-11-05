#include "../Test.h"
#include "../Fixture.h"
#include "application/jobs/JobQueue.h"
#include <cstdio>
using namespace exam;
namespace {
JobResult waitResult(JobQueue &queue, unsigned screen) {
    JobResult result;
    for (int i=0;i<500;++i) {
        if (queue.poll(screen,result)) return result;
        Sleep(10);
    }
    throw std::runtime_error("Фоновая операция не завершилась за 5 секунд");
}
}
void testJobs() {
    const char *path="bin/jobs-test.db"; std::remove(path);
    {
        Store main(path); prepareBank(main); addTask(main,"one");
        SessionRecord lesson; lesson.id="write"; lesson.studentId="student"; lesson.tasks={"one"};
        main.createSession(lesson); main.openAttempt(lesson.id,0,"one");
        Database failures(path);
        failures.script("CREATE TRIGGER reject_response BEFORE INSERT ON responses BEGIN SELECT RAISE(ABORT,'write rejected'); END;");
        JobQueue queue(path);
        queue.submit(1,[](Store &store,JobContext &) { store.saveProfile("worker","Профиль"); return std::string("saved"); });
        REQUIRE(waitResult(queue,1).success);
        REQUIRE(main.profiles().size()==2);
        queue.submit(2,[&main](Store &,JobContext &) { main.profiles(); return std::string("wrong"); });
        REQUIRE(!waitResult(queue,2).success);
        queue.submit(3,[](Store &store,JobContext &) {
            store.saveResponse(answerFor("failed-write","write:0","1")); return std::string("saved");
        });
        REQUIRE(!waitResult(queue,3).success);
        REQUIRE(main.responses("write:0").empty());
        unsigned cancel=queue.submit(4,[](Store &,JobContext &job) {
            for (;;) { job.checkpoint(); Sleep(1); }
            return std::string();
        });
        queue.cancel(cancel); REQUIRE(waitResult(queue,4).cancelled);
        queue.submit(5,[](Store &,JobContext &job) {
            for (;;) { job.checkpoint(); Sleep(1); }
            return std::string();
        });
        queue.leaveScreen(5);
        queue.submit(6,[](Store &store,JobContext &job) {
            store.replayModels([&job] { job.checkpoint(); }); return std::string("ready");
        });
        REQUIRE(waitResult(queue,6).success);
        JobResult stale; REQUIRE(!queue.poll(5,stale));
    }
    std::remove(path);
}
