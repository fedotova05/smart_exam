#include "ExamService.h"
#include "io/Json.h"
#include "io/Files.h"
#include <set>
namespace exam {
const std::vector<ExamCategory> &ogeCategories() {
    static const std::vector<ExamCategory> rows = {
        {1, 135, "1. Путешествия", "oge_practical"},
        {1, 134, "1. Планы участков и квартир", "oge_practical"},
        {1, 136, "1. Связь, шины, печи", "oge_practical"},
        {2, 133, "2. Путешествия", "oge_practical"},
        {2, 137, "2. Планы участков и квартир", "oge_practical"},
        {2, 138, "2. Связь, шины, печи", "oge_practical"},
        {3, 139, "3. Путешествия", "oge_practical"},
        {3, 140, "3. Планы участков и квартир", "oge_practical"},
        {3, 141, "3. Связь, шины, печи", "oge_practical"},
        {4, 142, "4. Путешествия", "oge_practical"},
        {4, 143, "4. Планы участков и квартир", "oge_practical"},
        {4, 144, "4. Связь, шины, печи", "oge_practical"},
        {5, 145, "5. Путешествия", "oge_practical"},
        {5, 146, "5. Планы участков и квартир", "oge_practical"},
        {5, 147, "5. Связь, шины, печи", "oge_practical"},
        {6, 6, "6. Обыкновенные дроби", "oge_fractions"},
        {6, 59, "6. Десятичные дроби", "oge_decimals"},
        {6, 129, "6. Смешанные вычисления", "oge_fractions"},
        {7, 51, "7. Числовые неравенства", "oge_comparison"},
        {7, 52, "7. Сравнение чисел", "oge_comparison"},
        {7, 91, "7. Числа на прямой", "oge_numberline"},
        {8, 54, "8. Степени и корни", "oge_expressions"},
        {9, 7, "9. Линейные уравнения", "oge_linear"},
        {9, 43, "9. Квадратные уравнения", "oge_quadratic"},
        {10, 154, "10. Диаграммы и деревья вероятностей", "oge_probability_tree"},
        {10, 20, "10. Вероятность", "oge_probability"},
        {10, 69, "10. Статистика", "oge_statistics"},
        {11, 8, "11. Графики функций", "oge_functions"},
        {12, 71, "12. Вычисление по формуле", "oge_formula"},
        {12, 27, "12. Выражение величины из формулы", "oge_linear"},
        {13, 55, "13. Линейные неравенства", "oge_inequality"},
        {13, 5, "13. Квадратные неравенства", "oge_quadratic_inequality"},
        {13, 3, "13. Системы неравенств", "oge_inequality_system"},
        {14, 113, "14. Арифметическая прогрессия", "oge_arithmetic_progression"},
        {14, 46, "14. Геометрическая прогрессия", "oge_geometric_progression"},
        {15, 57, "15. Углы треугольника", "oge_angles"},
        {15, 32, "15. Треугольники", "oge_triangles"},
        {15, 34, "15. Равнобедренный треугольник", "oge_isosceles"},
        {15, 11, "15. Прямоугольный треугольник", "oge_right_triangle"},
        {16, 12, "16. Углы в окружности", "oge_circle_angles"},
        {16, 35, "16. Касательные и хорды", "oge_circle_lines"},
        {16, 38, "16. Вписанная окружность", "oge_incircle"},
        {16, 36, "16. Описанная окружность", "oge_circumcircle"},
        {17, 33, "17. Ромб", "oge_rhombus"},
        {17, 42, "17. Прямоугольник", "oge_rectangle"},
        {17, 13, "17. Параллелограмм", "oge_parallelogram"},
        {17, 37, "17. Трапеция", "oge_trapezoid"},
        {18, 99, "18. Расстояния на решётке", "oge_grid_distance"},
        {18, 101, "18. Средняя линия", "oge_midline"},
        {18, 97, "18. Теорема Пифагора", "oge_pythagoras"},
        {18, 98, "18. Площади на решётке", "oge_grid_area"},
        {18, 96, "18. Длины на решётке", "oge_grid_distance"},
        {19, 15, "19. Геометрические утверждения", "oge_statements"}};
    return rows;
}
void installOgeCurriculum(Store &store) {
    auto data = parseJson(readFile("data/oge/curriculum.json"));
    std::set<std::string> known;
    for (const auto &s : store.skills())
        known.insert(s.id);
    std::set<std::string> topics;
    for (const auto &t : store.topics())
        topics.insert(t.first);
    for (const auto &topic : data.field("topics").array)
        if (!topics.count(topic.field("id").text()))
            store.saveTopic(topic.field("id").text(), topic.field("name").text());
    for (const auto &s : data.field("skills").array) {
        auto id = s.field("id").text();
        if (!known.count(id)) {
            store.saveSkill(id, s.field("topic").text(), s.field("name").text());
            store.saveSkillPosition(id, Point(s.field("x").integer(), s.field("y").integer()));
        }
    }
    for (const auto &s : data.field("skills").array) {
        auto id = s.field("id").text();
        if (!known.count(id))
            for (const auto &p : s.field("parents").array)
                store.savePrerequisite(id, p.text());
        Lecture lecture;
        lecture.id = "course-" + id;
        lecture.title = s.field("name").text();
        lecture.body = s.field("lecture").text();
        lecture.skills = {id};
        store.seedLecture(lecture);
        int index = 0;
        for (const auto &e : s.field("exercises").array) {
            TaskVersion t;
            t.id = "drill-" + id + "-" + std::to_string(++index);
            if (store.hasTask(t.id))
                continue;
            t.title = s.field("name").text() + " · " + std::to_string(index);
            t.difficulty = 550 + index * 20;
            TaskPart p;
            p.id = "answer";
            p.kind = PartKind::ShortAnswer;
            p.prompt = e.field("prompt").text();
            p.correctAnswer = e.field("answer").text();
            p.skills = {id};
            t.parts = {p};
            store.saveDraft(t);
            store.publish(t.id);
        }
    }
    SkillGraph checked(store.skillGraph());
}
} // namespace exam
