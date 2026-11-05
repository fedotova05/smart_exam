#include "../Test.h"
#include "../Fixture.h"
#include "application/SchoolService.h"
#include "application/training/TrainingService.h"
#include "io/Files.h"
#include <cstdio>
#include <set>
using namespace exam;
namespace {
void answer(SchoolService &school, const NextTask &task, bool correct) {
    for (const auto &part : task.task.parts) {
        AnswerPayload value;
        if (!correct)
            value.text = "999";
        else if (part.kind == PartKind::Expression)
            value.text = evaluateExact(parseExpression(part.expression)).text();
        else {
            for (const auto &root : solveEquation(part.expression).values) {
                if (!value.text.empty())
                    value.text += ";";
                value.text += root.text();
            }
        }
        school.submit(task.attempt.id, part.id, value, true);
    }
}
} // namespace
void testTraining() {
    // Ошибки ниже третьего уровня не должны возвращать бесконечное повторение темы.
    {
        Store store(":memory:");
        prepareBank(store);
        std::vector<std::string> chain = {"root", "middle", "lower", "basic", "elementary"};
        for (const auto &skill : chain) {
            store.saveSkill(skill, "algebra", skill);
            for (int i = 0; i < 6; ++i)
                addTask(store, skill + std::to_string(i), "x=1", skill, 800 + i);
        }
        for (std::size_t i = 0; i + 1 < chain.size(); ++i)
            store.setPrerequisites(chain[i], {chain[i + 1]});
        SchoolService school(store);
        school.startTopics("student", {"root"}, true, 1);
        NextTask current = school.next();
        answer(school, current, false);
        current = school.startSupport(current.attempt.id);
        // Сначала нижний навык проходит диагностику, но ошибка на закреплении среднего
        // снова направляет к его предпосылкам, уже на других заданиях.
        REQUIRE(store.trainingStep(current.attempt.id).skill == "middle");
        answer(school, current, false);
        school.finish(current.attempt.id);
        current = school.next();
        REQUIRE(store.trainingStep(current.attempt.id).skill == "lower");
        answer(school, current, true);
        school.finish(current.attempt.id);
        current = school.next();
        REQUIRE(store.trainingStep(current.attempt.id).stage == "practice");
        REQUIRE(store.trainingStep(current.attempt.id).skill == "middle");
        answer(school, current, false);
        school.finish(current.attempt.id);
        current = school.next();
        for (const std::string &skill : {"lower", "basic", "elementary"}) {
            REQUIRE(current.available && store.trainingStep(current.attempt.id).skill == skill);
            answer(school, current, false);
            school.finish(current.attempt.id);
            current = school.next();
        }
        REQUIRE(store.trainingStep(current.attempt.id).skill == "elementary");
        REQUIRE(store.trainingStep(current.attempt.id).stage == "practice");
        for (int i = 0; i < 2; ++i) {
            answer(school, current, true);
            school.finish(current.attempt.id);
            current = school.next();
        }
        for (const std::string &skill : {"basic", "lower", "middle"}) {
            REQUIRE(store.trainingStep(current.attempt.id).stage == "bridge");
            REQUIRE(store.trainingStep(current.attempt.id).skill == skill);
            answer(school, current, true);
            school.finish(current.attempt.id);
            current = school.next();
        }
        REQUIRE(store.trainingStep(current.attempt.id).stage == "return");
    }
    const std::string path = "bin/training-test.db";
    std::remove(path.c_str());
    std::string profile, original, interrupted;
    {
        Store store(path);
        SchoolService school(store);
        school.addStarterContent();
        auto demo = school.history("demo-training");
        REQUIRE(demo.size() == 9);
        REQUIRE(demo.front().hadErrors && !demo.back().hadErrors);
        REQUIRE(demo[0].taskId == demo[7].taskId);
        REQUIRE(demo[8].taskId != demo[0].taskId);
        REQUIRE(school.snapshot("demo-training").skills["q_roots"].observations == 2);
        REQUIRE(store.observationCount(demo[7].attemptId) == 0);
        // Обычное задание учителя, а не встроенный элемент демонстрации.
        TaskVersion borrowed = school.task("course-b_signs-1");
        borrowed.id = "teacher-signs";
        borrowed.published = false;
        borrowed.title = "Пример учителя со знаками";
        borrowed.difficulty = 100;
        borrowed.parts[0].expression = "(-2)*7";
        school.saveDraft(borrowed);
        school.publish(borrowed.id);
        profile = school.createProfile("Проверка маршрута");
        school.startTopics(profile, {"q_roots"}, true, 1);
        NextTask current = school.next();
        original = current.task.id;
        REQUIRE(current.task.parts[0].skills == std::vector<std::string>({"q_roots"}));
        answer(school, current, false);
        REQUIRE(school.canSupport(current.attempt.id));
        REQUIRE(school.next().attempt.id == current.attempt.id);
        current = school.startSupport(current.attempt.id);
        REQUIRE(store.trainingStep(current.attempt.id).skill == "q_coefficients");
        REQUIRE(!school.snapshot(profile).skills["b_signs"].known);
        REQUIRE(current.task.parts[0].displayExpression == "2*x^2-3*x-2=0");
        answer(school, current, true);
        school.finish(current.attempt.id);
        current = school.next();
        REQUIRE(store.trainingStep(current.attempt.id).skill == "q_discriminant");
        answer(school, current, false);
        school.finish(current.attempt.id);
        current = school.next();
        REQUIRE(current.task.id == "teacher-signs");
        answer(school, current, false);
        school.finish(current.attempt.id);
        current = school.next();
        REQUIRE(store.trainingStep(current.attempt.id).stage == "practice");
        interrupted = current.attempt.id;
    }
    {
        Store store(path);
        SchoolService school(store);
        REQUIRE(school.resume(profile));
        NextTask current = school.next();
        REQUIRE(current.attempt.id == interrupted);
        REQUIRE(school.latestLesson(profile).settings.targetSkills ==
                std::vector<std::string>({"q_roots"}));
        for (int i = 0; i < 2; ++i) {
            REQUIRE(store.trainingStep(current.attempt.id).stage == "practice");
            answer(school, current, true);
            school.finish(current.attempt.id);
            current = school.next();
        }
        REQUIRE(store.trainingStep(current.attempt.id).stage == "bridge");
        answer(school, current, true);
        school.finish(current.attempt.id);
        current = school.next();
        REQUIRE(current.task.id == original &&
                store.trainingStep(current.attempt.id).stage == "return");
        int observations = school.snapshot(profile).skills["q_roots"].observations;
        answer(school, current, false);
        answer(school, current, true);
        school.finish(current.attempt.id);
        REQUIRE(school.snapshot(profile).skills["q_roots"].observations == observations);
        current = school.next();
        REQUIRE(store.trainingStep(current.attempt.id).stage == "verify" &&
                current.task.id != original);
        answer(school, current, true);
        school.finish(current.attempt.id);
        REQUIRE(!school.next().available);
        REQUIRE(school.snapshot(profile).skills["q_roots"].observations == observations + 1);
        // Несколько выбранных навыков; набор проверки зафиксирован до первого ответа.
        std::string test = school.createProfile("Проверка знаний");
        school.startTopics(test, {"b_signs", "q_roots"}, false, 3);
        auto fixed = school.latestLesson(test);
        REQUIRE(fixed.tasks.size() == 3);
        for (const auto &id : fixed.tasks)
            for (const auto &part : school.task(id).parts)
                for (const auto &skill : part.skills)
                    REQUIRE(skill == "b_signs" || skill == "q_roots");
        current = school.next();
        answer(school, current, false);
        REQUIRE(!school.canSupport(current.attempt.id));
        school.finish(current.attempt.id);
        REQUIRE(school.progress(test).hidden);
        REQUIRE(school.latestLesson(test).tasks == fixed.tasks);
        REQUIRE(school.reviewSkills(test).empty());
        for (int i = 1; i < 3; ++i) {
            current = school.next();
            REQUIRE(current.task.id == fixed.tasks[i]);
            answer(school, current, true);
            school.finish(current.attempt.id);
        }
        REQUIRE(!school.next().available);
        REQUIRE(!school.reviewSkills(test).empty());
        // Не хватает упражнений: честное завершение без случайного перехода в другую тему.
        std::string limited = school.createProfile("Пустой банк поддержки");
        school.startTopics(limited, {"q_roots"}, true, 1);
        current = school.next();
        answer(school, current, false);
        for (const auto &task : TrainingService(store).scopedTasks({"q_coefficients"}))
            school.archive(task.id);
        NextTask unavailable = school.startSupport(current.attempt.id);
        REQUIRE(!unavailable.available);
        REQUIRE(unavailable.reason.find("нет новых") != std::string::npos);
    }
    std::remove(path.c_str());
    {
        Store store(":memory:");
        SchoolService school(store);
        school.addStarterContent();
        std::string learner = school.createProfile("Ограничение повторов");
        school.startTopics(learner, {"b_signs"}, true, 1);
        NextTask current = school.next();
        answer(school, current, false);
        current = school.startSupport(current.attempt.id);
        int steps = 0;
        while (current.available && steps < 10) {
            answer(school, current, false);
            school.finish(current.attempt.id);
            current = school.next();
            ++steps;
        }
        REQUIRE(!current.available && steps == 2);
        REQUIRE(current.reason.find("перерыв") != std::string::npos);
    }
    // Обновление базы пятой версии сохраняет старые данные и добавляет пустую формулу условия.
    const std::string migration = "bin/migration-training.db";
    std::remove(migration.c_str());
    {
        Database db(migration);
        for (const std::string &file :
             {"001_initial.sql", "002_attempt_snapshot.sql", "003_session_completion.sql",
              "004_interface_data.sql", "005_graph_settings.sql"})
            db.script(readFile("db/migrations/" + file));
        db.query("INSERT INTO profiles VALUES('old','Старый профиль')");
    }
    {
        Store upgraded(migration);
        REQUIRE(upgraded.profiles()[0].first == "old");
        SchoolService school(upgraded);
        school.addStarterContent();
        REQUIRE(school.task("course-coefficients-1").parts[0].displayExpression == "2*x^2-3*x-2=0");
    }
    std::remove(migration.c_str());
}
