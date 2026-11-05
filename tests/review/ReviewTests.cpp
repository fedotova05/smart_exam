#include "../Test.h"
#include "../Fixture.h"
#include "application/review/ReviewService.h"
#include "application/session/SessionService.h"
#include "adaptation/Adaptation.h"
#include "math/checkers/EquationChecker.h"
#include <cmath>
using namespace exam;
void testReview() {
    Store store(":memory:"); prepareBank(store); addTask(store,"shared");
    store.saveProfile("other","Второй ученик");
    UnavailableGraphChecker graph; EquationChecker checker(graph);
    Adaptation model(store.modelParameters(),SkillGraph(store.skillGraph()));
    SessionService service(store,checker,&model,&model);
    for (int i=0;i<2;++i) {
        SessionRecord lesson; lesson.id="review-"+std::to_string(i);
        lesson.studentId=i ? "other" : "student"; lesson.tasks={"shared"};
        service.start(lesson); auto next=service.next(lesson.id);
        service.submitResponse(answerFor("answer-"+std::to_string(i),next.attempt.id,i ? "1" : "0"));
        service.finishAttempt(next.attempt.id);
    }
    auto original=store.attemptSnapshot("review-1:0");
    double otherBefore=store.snapshot("other").skills["linear"].rating;
    ReviewService review(store); ReviewVerdict verdict;
    verdict.requestId="review"; verdict.attemptId="review-0:0"; verdict.partId="answer";
    verdict.reviewer="KChernmo"; verdict.reason="Исправление ошибочного автоматического вердикта";
    verdict.status=CheckStatus::Correct;
    review.revise(verdict);
    REQUIRE(store.attempt(verdict.attemptId).score==1);
    REQUIRE(store.attempts("review-0").size()==1);
    REQUIRE(store.snapshot("student").skills["linear"].observations==1);
    REQUIRE(store.snapshot("other").skills["linear"].rating!=otherBefore);
    REQUIRE(store.attemptSnapshot("review-1:0").skills["linear"].rating==original.skills["linear"].rating);
    REQUIRE(store.responses(verdict.attemptId)[0].result.status==CheckStatus::Incorrect);
    double rating=store.taskRating("shared"), mastery=store.snapshot("student").skills["linear"].mastery;
    review.replay(); review.revise(verdict);
    REQUIRE(store.taskRating("shared")==rating);
    REQUIRE(store.snapshot("student").skills["linear"].mastery==mastery);
    REQUIRE(review.history(verdict.attemptId).size()==1);
    bool failed=false;
    try { verdict.requestId="empty-reason"; verdict.reason="  "; review.revise(verdict); }
    catch (const std::exception &) { failed=true; }
    REQUIRE(failed); REQUIRE(review.history(verdict.attemptId).size()==1);
    verdict.requestId="second-review"; verdict.reason="Нужна дополнительная проверка";
    verdict.status=CheckStatus::Unsupported; review.revise(verdict);
    REQUIRE(store.attempt(verdict.attemptId).provisional);
    REQUIRE(!store.snapshot("student").skills["linear"].known);
    REQUIRE(review.history(verdict.attemptId).size()==2);
}
