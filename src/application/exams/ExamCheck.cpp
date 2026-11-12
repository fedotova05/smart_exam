#include "ExamService.h"
#include "io/Files.h"
#include <iostream>
#include <set>
#include <chrono>
namespace exam {
int checkExamSources(bool all) {
    makeDirectories("bin");
    auto path = "bin/" + newId("oge-source-check") + ".db";
    writeFile("bin/oge-source-check-path.txt", path);
    Store store(path);
    ExamService service(store);
    SchoolService school(store);
    std::set<int> numbers;
    int failures = 0;
    for (const auto &category : ogeCategories()) {
        if (!numbers.insert(category.number).second && !all)
            continue;
        auto start = std::chrono::steady_clock::now();
        auto result = service.load(category.id, 1, true, [start] {
            if (std::chrono::steady_clock::now() - start > std::chrono::seconds(45))
                throw std::runtime_error("Source timeout");
        });
        std::cout << "Number " << category.number << ": " << result.tasks.size() << " tasks, "
                  << result.added << " added\n";
        for (auto &w : result.warnings)
            std::cout << "  " << w << "\n";
        if (result.tasks.empty()) {
            ++failures;
            continue;
        }
        auto task = store.task(result.tasks[0]);
        std::cout << "  " << task.id << "; images=" << task.resources.size()
                  << "; skill=" << task.parts[0].skills[0] << "\n";
        auto student =
            school.createProfile("Проверка источника " + std::to_string(category.number));
        school.start(student, SessionSettings(), {task.id});
        auto next = school.next();
        AnswerPayload answer;
        answer.text = task.parts[0].correctAnswer;
        if (school.submit(next.attempt.id, task.parts[0].id, answer, true).status !=
            CheckStatus::Correct)
            ++failures;
        school.finish(next.attempt.id);
        std::cout.flush();
    }
    std::cout << "Source check failures: " << failures << "\n";
    return failures ? 1 : 0;
}
} // namespace exam
