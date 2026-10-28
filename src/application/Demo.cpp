#include "Demo.h"
#include "session/SessionService.h"
#include "adaptation/Adaptation.h"
#include "math/checkers/EquationChecker.h"
#include <iostream>
namespace exam {
int runDemo() {
    try {
        Store store(":memory:");
        store.saveProfile("demo", "Учебный профиль");
        store.saveTopic("algebra", "Алгебра");
        store.saveSkill("linear", "algebra", "Линейные уравнения");
        const char *ids[] = {"start", "easy", "hard"};
        const char *equations[] = {"2*x+2=4", "x+1=2", "3*x-2=1"};
        const double difficulty[] = {1000, 900, 1100};
        for (int i = 0; i < 3; ++i) {
            TaskVersion task;
            task.id = ids[i];
            task.title = "Линейное уравнение";
            task.difficulty = difficulty[i];
            TaskPart part;
            part.id = "answer";
            part.prompt = "Решите уравнение";
            part.expression = equations[i];
            part.skills = {"linear"};
            task.parts.push_back(part);
            store.saveDraft(task);
            store.publish(task.id);
        }
        UnavailableGraphChecker graph;
        EquationChecker checker(graph);
        Adaptation adaptation(store.modelParameters(), SkillGraph(store.skillGraph()));
        SessionService service(store, checker, &adaptation, &adaptation);
        SessionRecord session;
        session.id = "demo";
        session.studentId = "demo";
        session.settings.mode = SessionMode::Dynamic;
        service.start(session);
        NextTask current = service.next(session.id);
        std::cout << "Задание: " << current.task.parts[0].expression << '\n';
        ResponseRecord answer;
        answer.requestId = "wrong";
        answer.attemptId = current.attempt.id;
        answer.partId = "answer";
        answer.answer.text = "2";
        std::cout << "Ответ 2: " << service.submitResponse(answer).message << '\n';
        answer.requestId = "corrected";
        answer.answer.text = "1";
        std::cout << "Исправление на 1: " << service.submitResponse(answer).message << '\n';
        std::cout << "Балл: " << service.finishAttempt(current.attempt.id).score << '\n';
        SkillState state = store.snapshot("demo").skills["linear"];
        std::cout << "После самостоятельной ошибки: BKT=" << state.mastery
                  << ", Elo=" << state.rating << '\n';
        NextTask next = service.next(session.id);
        std::cout << "Следующее: " << next.task.parts[0].expression << " (" << next.reason << ")\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
} // namespace exam
