#ifndef SMARTEXAM_FIXTURE_H
#define SMARTEXAM_FIXTURE_H
#include "storage/Store.h"
inline void prepareBank(exam::Store& store) {
    store.saveProfile("student","Тестовая ученица");
    store.saveTopic("algebra","Алгебра");
    for(const std::string& skill:{"fractions","linear","quadratic","line_graph","parabola_graph"}) store.saveSkill(skill,"algebra",skill);
}
inline exam::TaskVersion addTask(exam::Store& store,const std::string& id,const std::string& equation="x+1=2",const std::string& skill="linear",double difficulty=1000) {
    exam::TaskVersion task; task.id=id; task.title="Тестовое задание"; task.difficulty=difficulty;
    exam::TaskPart part; part.id="answer"; part.prompt="Решите уравнение"; part.expression=equation; part.skills.push_back(skill);
    task.parts.push_back(part); store.saveDraft(task); store.publish(id); return store.task(id);
}
inline exam::ResponseRecord answerFor(const std::string& request,const std::string& attempt,const std::string& text) {
    exam::ResponseRecord response; response.requestId=request; response.attemptId=attempt;
    response.partId="answer"; response.answer.text=text; return response;
}
#endif
