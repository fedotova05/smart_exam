#include "../Test.h"
#include "../Fixture.h"
#include "application/SchoolService.h"
using namespace exam;
void testDemonstration() {
    Store store(":memory:");
    SchoolService school(store);
    school.addDemonstrationCourse();
    REQUIRE(school.tasks().size() >= 49);
    REQUIRE(!store.hasTeacherPassword());
    REQUIRE(store.skills().size() == 14);
    SkillGraph graph(store.skillGraph());
    REQUIRE(school.history("demo-check").size() == 3);
    REQUIRE(school.progress("demo-check").score == 2);
    REQUIRE(school.resume("demo-pause"));
    REQUIRE(school.lessonTime().paused);
    school.continueLesson();
    auto resumed = school.next();
    REQUIRE(resumed.available);
    REQUIRE(school.answerDrafts(resumed.attempt.id)["a"].text == "-2;2");
    bool failedPractice = false, lowerAfterFailure = false, returned = false;
    for (const auto &attempt : store.attempts(school.latestLesson("demo-deep").id)) {
        auto step = store.trainingStep(attempt.id);
        if (step.skill == "b_signs" && step.stage == "practice" && attempt.score == 0)
            failedPractice = true;
        if (failedPractice && step.skill == "b_integers")
            lowerAfterFailure = true;
        if (step.stage == "return" && attempt.score > 0)
            returned = true;
    }
    REQUIRE(failedPractice && lowerAfterFailure && returned);
    loginTeacherForTest(store);
    REQUIRE(school.reviewItems("demo-review", true).size() == 1);
    REQUIRE(school.lessonTemplates().size() == 2);
    auto count = school.tasks().size();
    school.addDemonstrationCourse();
    REQUIRE(school.tasks().size() == count);
}
