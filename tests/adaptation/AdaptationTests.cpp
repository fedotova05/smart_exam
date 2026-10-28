#include "../Test.h"
#include "../Fixture.h"
#include "adaptation/Adaptation.h"
#include "application/session/SessionService.h"
#include "math/checkers/EquationChecker.h"
#include <cmath>
#include <cstdio>
using namespace exam;
namespace {
void setupSelectionBank(Store &store) {
    prepareBank(store);
    addTask(store, "start", "x+1=2", "linear", 1000);
    addTask(store, "easy", "x=1", "linear", 900);
    addTask(store, "hard", "2x=2", "linear", 1100);
}
std::string choiceAfter(bool correct, bool fixed) {
    Store store(":memory:");
    setupSelectionBank(store);
    UnavailableGraphChecker graph;
    EquationChecker checker(graph);
    Adaptation model(store.modelParameters(), SkillGraph(store.skillGraph()));
    SessionService service(store, checker, &model, &model);
    SessionRecord session;
    session.id = "s";
    session.studentId = "student";
    session.settings.mode = fixed ? SessionMode::Fixed : SessionMode::Dynamic;
    if (fixed)
        session.tasks = {"start", "hard"};
    service.start(session);
    NextTask next = service.next("s");
    REQUIRE(next.task.id == "start");
    REQUIRE(!store.attemptSnapshot(next.attempt.id).skills["linear"].known);
    service.submitResponse(answerFor("response", next.attempt.id, correct ? "1" : "2"));
    REQUIRE(!store.snapshot("student").skills["linear"].known);
    REQUIRE(service.next("s").task.id == "start");
    service.finishAttempt(next.attempt.id);
    REQUIRE(store.snapshot("student").skills["linear"].observations == 1);
    service.finishAttempt(next.attempt.id);
    REQUIRE(store.observationCount(next.attempt.id) == 1);
    REQUIRE(store.snapshot("student").skills["linear"].observations == 1);
    REQUIRE(!store.attemptSnapshot(next.attempt.id).skills["linear"].known);
    return service.next("s").task.id;
}
} // namespace
void testAdaptation() {
    Store store(":memory:");
    prepareBank(store);
    store.savePrerequisite("quadratic", "linear");
    Adaptation model(store.modelParameters(), SkillGraph(store.skillGraph()));
    SkillState before;
    SkillState success = model.update(before, true, 1000),
               failure = model.update(before, false, 1000);
    REQUIRE(std::abs(success.mastery - 0.5764705882352941) < 1e-10);
    REQUIRE(std::abs(failure.mastery - 0.1272727272727273) < 1e-10);
    REQUIRE(success.rating == 1016 && failure.rating == 984);
    StudentSnapshot snapshot;
    PartEvidence part;
    part.skills = {"linear"};
    part.finalStatus = CheckStatus::Correct;
    auto observations = model.observe(snapshot, {part, part}, 1000);
    REQUIRE(observations.size() == 1);
    REQUIRE(observations[0].after.observations == 1);
    part.usedHelp = true;
    observations = model.observe(snapshot, {part}, 1000);
    REQUIRE(!observations[0].eligible && !observations[0].after.known);
    part.usedHelp = false;
    part.finalStatus = CheckStatus::Unsupported;
    REQUIRE(!model.observe(snapshot, {part}, 1000)[0].eligible);
    part.finalStatus = CheckStatus::Correct;
    part.hadError = true;
    REQUIRE(model.observe(snapshot, {part}, 1000)[0].eligible);
    REQUIRE(!model.observe(snapshot, {part}, 1000)[0].correct);
    bool rejected = false;
    try {
        SkillGraph invalid({{"a", {"b"}}, {"b", {"a"}}});
    } catch (const std::invalid_argument &) {
        rejected = true;
    }
    REQUIRE(rejected);
    rejected = false;
    try {
        SkillGraph invalid(std::map<std::string, std::vector<std::string>>{{"a", {"missing"}}});
    } catch (const std::invalid_argument &) {
        rejected = true;
    }
    REQUIRE(rejected);
    TaskVersion quadratic = addTask(store, "quadratic", "x^2=1", "quadratic");
    REQUIRE(model.selectNext(snapshot, {quadratic}, {}).found);
    quadratic.published = false;
    REQUIRE(!model.selectNext(snapshot, {quadratic}, {}).found);
    quadratic.published = true;
    quadratic.archived = true;
    REQUIRE(!model.selectNext(snapshot, {quadratic}, {}).found);
    REQUIRE(!model.selectNext(snapshot, {}, {}).found);
    REQUIRE(choiceAfter(false, false) == "easy");
    REQUIRE(choiceAfter(true, false) == "hard");
    REQUIRE(choiceAfter(false, true) == "hard");

    const std::string path = "bin/adaptation-test.db";
    std::remove(path.c_str());
    {
        Store persistent(path);
        prepareBank(persistent);
        addTask(persistent, "one");
        UnavailableGraphChecker graph;
        EquationChecker checker(graph);
        Adaptation learning(persistent.modelParameters(), SkillGraph(persistent.skillGraph()));
        SessionService service(persistent, checker, &learning, &learning);
        SessionRecord session;
        session.id = "help";
        session.studentId = "student";
        session.tasks = {"one"};
        service.start(session);
        NextTask next = service.next("help");
        service.requestHelp("hint", next.attempt.id, "answer", "hint");
        service.submitResponse(answerFor("final", next.attempt.id, "1"));
        REQUIRE(service.finishAttempt(next.attempt.id).score == 1);
        REQUIRE(!persistent.snapshot("student").skills["linear"].known);
        REQUIRE(persistent.observationCount(next.attempt.id) == 1);
    }
    {
        Store reopened(path);
        REQUIRE(reopened.observationCount("help:0") == 1);
        REQUIRE(!reopened.snapshot("student").skills["linear"].known);
    }
    std::remove(path.c_str());
    Store limited(":memory:");
    prepareBank(limited);
    addTask(limited, "only");
    UnavailableGraphChecker graph;
    EquationChecker checker(graph);
    Adaptation learning(limited.modelParameters(), SkillGraph(limited.skillGraph()));
    SessionService service(limited, checker, &learning, &learning);
    SessionRecord session;
    session.id = "limited";
    session.studentId = "student";
    session.settings.mode = SessionMode::Dynamic;
    session.settings.feedback = FeedbackPolicy::AfterSet;
    service.start(session);
    NextTask next = service.next(session.id);
    service.submitResponse(answerFor("single", next.attempt.id, "1"));
    service.finishAttempt(next.attempt.id);
    REQUIRE(!service.result(next.attempt.id).visible);
    REQUIRE(!service.next(session.id).available);
    REQUIRE(service.result(next.attempt.id).visible);
    REQUIRE(!limited.attempt(next.attempt.id).selectionReason.empty());
}
