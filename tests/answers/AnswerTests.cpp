#include "../Test.h"
#include "../Fixture.h"
#include "application/SchoolService.h"
#include "io/packages/PackageService.h"
using namespace exam;
void testAnswers() {
    Store store(":memory:");
    prepareBank(store);
    SchoolService school(store);
    UnavailableGraphChecker graph;
    EquationChecker checker(graph);
    TaskPart part;
    AnswerPayload answer;
    part.kind = PartKind::Number;
    part.expression = "sqrt(2)";
    part.decimalPlaces = 2;
    answer.text = "1.41";
    REQUIRE(checker.checkAnswer(part, answer).status == CheckStatus::Correct);
    answer.text = "1.42";
    REQUIRE(checker.checkAnswer(part, answer).status == CheckStatus::Incorrect);
    part.decimalPlaces = -1;
    part.answerTolerance = 0.01;
    answer.text = "1.42";
    REQUIRE(checker.checkAnswer(part, answer).status == CheckStatus::Correct);
    part.kind = PartKind::Polynomial;
    part.expression = "(x+1)^2";
    part.answerTolerance = 0;
    answer.text = "x^2+2*x+1";
    REQUIRE(checker.checkAnswer(part, answer).status == CheckStatus::Correct);
    answer.text = "x^2+1";
    REQUIRE(checker.checkAnswer(part, answer).status == CheckStatus::Incorrect);
    part.kind = PartKind::Choice;
    part.expression = "";
    part.choices = {"Первый", "Второй", "Третий"};
    part.correctChoices = {1, 3};
    answer.text = "3;1";
    REQUIRE(checker.checkAnswer(part, answer).status == CheckStatus::Correct);
    answer.text = "1;1;3";
    REQUIRE(checker.checkAnswer(part, answer).status == CheckStatus::InvalidInput);
    answer.text = "1;2";
    REQUIRE(checker.checkAnswer(part, answer).status == CheckStatus::Incorrect);
    answer.text = "1;";
    REQUIRE(checker.checkAnswer(part, answer).status == CheckStatus::InvalidInput);
    part.id = "answer";
    part.prompt = "Выберите";
    part.skills = {"linear"};
    TaskVersion choice;
    choice.id = "choices";
    choice.title = "Выбор";
    choice.parts = {part};
    school.saveDraft(choice);
    school.publish(choice.id);
    REQUIRE(store.task(choice.id).parts[0].correctChoices == part.correctChoices);
    PackageService packages(school, "bin");
    packages.exportPackage("bin/answer-package.json", {choice.id});
    auto imported = packages.inspect("bin/answer-package.json");
    REQUIRE(imported.items[0].task.parts[0].choices == part.choices);
    REQUIRE(imported.items[0].task.parts[0].correctChoices == part.correctChoices);
    TaskVersion steps = choice;
    steps.id = "steps";
    steps.parts[0] = TaskPart();
    steps.parts[0].id = "answer";
    steps.parts[0].prompt = "Решите";
    steps.parts[0].expression = "2*x=4";
    steps.parts[0].skills = {"linear"};
    steps.parts[0].requireSteps = true;
    school.saveDraft(steps);
    school.publish(steps.id);
    SessionService service(store, checker);
    SessionRecord lesson;
    lesson.id = "required";
    lesson.studentId = "student";
    lesson.tasks = {steps.id};
    service.start(lesson);
    auto next = service.next(lesson.id);
    REQUIRE(service.submitResponse(answerFor("only-final", next.attempt.id, "2")).status ==
            CheckStatus::Incomplete);
    bool blocked = false;
    try {
        service.finishAttempt(next.attempt.id);
    } catch (const std::exception &) {
        blocked = true;
    }
    REQUIRE(blocked && !store.attempt(next.attempt.id).finished);
    auto step = answerFor("step", next.attempt.id, "x=2");
    step.finalAnswer = false;
    step.operation = "divide";
    step.operand = "2";
    REQUIRE(service.submitResponse(step).status == CheckStatus::Correct);
    auto final = answerFor("final", next.attempt.id, "2");
    REQUIRE(service.submitResponse(final).status == CheckStatus::Correct);
    REQUIRE(service.submitResponse(final).status == CheckStatus::Correct);
    REQUIRE(service.finishAttempt(next.attempt.id).score == 1);
    auto wrongAction = checker.checkStep("2*x=4", "x=2", "subtract", "2");
    REQUIRE(wrongAction.status == CheckStatus::Incorrect);
    REQUIRE(wrongAction.message.find("действие") != std::string::npos);
    REQUIRE(checker.checkStep("2*x=4", "x=3", "divide", "2").message.find("множество") !=
            std::string::npos);
    steps.id = "bad";
    steps.parts[0].kind = static_cast<PartKind>(99);
    steps.parts[0].requireSteps = false;
    school.saveDraft(steps);
    blocked = false;
    try {
        store.publish(steps.id);
    } catch (const std::exception &) {
        blocked = true;
    }
    REQUIRE(blocked);
    steps.id = "quadratic-steps";
    steps.parts[0].kind = PartKind::Equation;
    steps.parts[0].expression = "x^2=1";
    steps.parts[0].requireSteps = true;
    REQUIRE(!school.validate(steps).empty());
}
