#include "../Test.h"
#include "../Fixture.h"
#include "application/session/SessionService.h"
#include "math/checkers/EquationChecker.h"
using namespace exam;
namespace {
class CountingSelector : public TaskSelector {
public:
    mutable int calls;
    CountingSelector(): calls(0) {}
    Selection selectNext(const StudentSnapshot&,const std::vector<TaskVersion>& tasks,const std::vector<std::string>&) const override {
        ++calls; Selection result; if(!tasks.empty()) { result.found=true; result.task=tasks[0]; } return result;
    }
};
}
void testSession() {
    UnavailableGraphChecker graph; EquationChecker checker(graph);
    for(FeedbackPolicy feedback:{FeedbackPolicy::AfterStep,FeedbackPolicy::AfterTask,FeedbackPolicy::AfterSet}) {
        Store store(":memory:"); prepareBank(store); addTask(store,"first"); addTask(store,"second","x=3");
        SessionService service(store,checker);
        SessionRecord session; session.id="fixed"; session.studentId="student"; session.tasks={"first","second"}; session.settings.feedback=feedback;
        service.start(session); NextTask current=service.next("fixed");
        REQUIRE(current.task.id=="first");
        REQUIRE(!store.attemptSnapshot(current.attempt.id).skills["linear"].known);
        ResponseRecord wrong=answerFor("wrong",current.attempt.id,"x=9"); wrong.finalAnswer=false;
        CheckResult checked=service.submitResponse(wrong);
        REQUIRE(checked.status==(feedback==FeedbackPolicy::AfterStep?CheckStatus::Incorrect:CheckStatus::Incomplete));
        ResponseRecord step=answerFor("step",current.attempt.id,"x=1"); step.finalAnswer=false;
        service.submitResponse(step);
        ResponseRecord final=answerFor("final",current.attempt.id,"1");
        service.submitResponse(final); service.submitResponse(final);
        REQUIRE(service.history(current.attempt.id).size()==3);
        AttemptResult result=service.finishAttempt(current.attempt.id);
        REQUIRE(result.visible==(feedback!=FeedbackPolicy::AfterSet));
        if(result.visible) { REQUIRE(result.score==1); REQUIRE(!result.provisional); }
        REQUIRE(service.finishAttempt(current.attempt.id).score==result.score);
        REQUIRE(service.submitResponse(final).status==(feedback==FeedbackPolicy::AfterSet?CheckStatus::Incomplete:CheckStatus::Correct));
        bool rejected=false; final.answer.text="2";
        try { service.submitResponse(final); } catch(const std::runtime_error&) { rejected=true; } REQUIRE(rejected);
        addTask(store,"new"); store.archive("second");
        NextTask second=service.next("fixed"); REQUIRE(second.task.id=="second");
        service.submitResponse(answerFor("second-final",second.attempt.id,"3")); service.finishAttempt(second.attempt.id);
        REQUIRE(service.result(current.attempt.id).visible); REQUIRE(service.result(current.attempt.id).score==1);
        REQUIRE(!service.next("fixed").available);
    }
    Store store(":memory:"); prepareBank(store); addTask(store,"one");
    CountingSelector selector; SessionService service(store,checker,&selector);
    SessionRecord dynamic; dynamic.id="dynamic"; dynamic.studentId="student"; dynamic.settings.mode=SessionMode::Dynamic; dynamic.settings.taskLimit=2;
    service.start(dynamic); NextTask first=service.next("dynamic");
    REQUIRE(selector.calls==1); REQUIRE(service.next("dynamic").attempt.id==first.attempt.id); REQUIRE(selector.calls==1);
    service.requestHelp("help",first.attempt.id,"answer","hint"); service.requestHelp("help",first.attempt.id,"answer","hint");
    REQUIRE(store.usedHelp(first.attempt.id,"answer"));
    REQUIRE(service.finishAttempt(first.attempt.id).provisional);
    REQUIRE(service.next("dynamic").attempt.id!=first.attempt.id); REQUIRE(selector.calls==2);
    service.finishAttempt("dynamic:1");
    TaskVersion graphTask; graphTask.id="graph"; graphTask.title="График";
    TaskPart part; part.id="answer"; part.kind=PartKind::Line; part.prompt="Постройте"; part.expression="x+1"; part.skills={"line_graph"}; graphTask.parts.push_back(part);
    store.saveDraft(graphTask); store.publish("graph");
    SessionRecord graphic; graphic.id="graphic"; graphic.studentId="student"; graphic.tasks={"graph"}; service.start(graphic);
    NextTask next=service.next("graphic"); ResponseRecord response=answerFor("graph-answer",next.attempt.id,"");
    response.answer.graph.points.push_back(Point(1.25,2.5)); response.answer.graph.coefficients={1,1};
    REQUIRE(service.submitResponse(response).status==CheckStatus::Unsupported);
    REQUIRE(service.history(next.attempt.id)[0].answer.graph.points[0].x==1.25);
    REQUIRE(service.finishAttempt(next.attempt.id).provisional);
}
