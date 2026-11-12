#include "../Test.h"
#include "../Fixture.h"
#include "application/exams/ExamService.h"
#include "io/Html.h"
#include "io/Https.h"
#include "io/Files.h"
#include "io/SvgImage.h"
#include <set>
#include <cstdio>
#include <chrono>
using namespace exam;
namespace {
template <class F> bool fails(F f) {
    try {
        f();
        return false;
    } catch (const std::exception &) {
        return true;
    }
}
std::string card(const std::string &answer = "05") {
    return "<div class='prob_maindiv'><div class='prob_nums'>Тип 9 № <a "
           "href='/problem?id=12345'>12345</a></div>"
           "<div class='pbody'>Общее условие <b>с рисунком</b>.</div>"
           "<div class='pbody'>Найдите x: x − 5 = 0.<img src='/get_file?id=1' alt='Схема'></div>"
           "<div class=\"solution\"><div class='pbody'>НЕ ПОКАЗЫВАТЬ РЕШЕНИЕ</div></div>"
           "<div class='answer'>Ответ: " +
           answer + ".</div></div>";
}
} // namespace
void testMaterials() {
    Store store(":memory:");
    SchoolService school(store);
    installOgeCurriculum(store);
    auto count = store.publishedTasks().size();
    installOgeCurriculum(store);
    REQUIRE(store.publishedTasks().size() == count);
    REQUIRE(count == 212 && store.lectures().size() == 53);
    std::set<int> numbers;
    for (auto &c : ogeCategories()) {
        numbers.insert(c.number);
        REQUIRE(store.skillGraph().count(c.skill));
    }
    REQUIRE(numbers.size() == 19 && *numbers.begin() == 1 && *numbers.rbegin() == 19);
    std::map<std::string, int> exercises;
    for (auto &t : store.publishedTasks()) {
        REQUIRE(school.validate(t).empty());
        for (auto &p : t.parts)
            for (auto &s : p.skills)
                ++exercises[s];
    }
    for (auto &s : store.skills())
        REQUIRE(exercises[s.id] >= 4);
    REQUIRE(fails([&] { school.saveLecture(Lecture()); }));
    loginTeacherForTest(store);
    auto local = ExamService(store).importPackage("data/samples/oge-2026.json");
    REQUIRE(local.warnings.empty() && local.tasks.size() == 19);
    auto repeated = ExamService(store).importPackage("data/samples/oge-2026.json");
    REQUIRE(repeated.tasks == local.tasks && repeated.added == 0);
    Lecture v;
    v.id = "teacher-lecture";
    v.title = "Дроби";
    v.body = "Первый абзац.\nВторой абзац.";
    v.skills = {"oge_fractions"};
    school.saveLecture(v);
    REQUIRE(school.lectures(true).size() == store.lectures().size() + 1);
    REQUIRE(fails([&] { school.lecture(v.id); }));
    v = school.lecture(v.id, true);
    v.published = true;
    school.saveLecture(v);
    REQUIRE(school.lecture(v.id).revision == 2);
    REQUIRE(fails([&] { school.saveLecture(v); }));
    v = school.lecture(v.id);
    v.skills = {"missing"};
    REQUIRE(fails([&] { school.saveLecture(v); }));
    REQUIRE(school.lecture(v.id).skills[0] == "oge_fractions");
    v = school.lecture(v.id);
    v.attachment = "lectures/../../secret.pdf";
    REQUIRE(fails([&] { school.saveLecture(v); }));
    writeFile("bin/materials-attachment.pdf", "%PDF-1.4\n%%EOF\n");
    v = school.lecture(v.id);
    school.saveLecture(v, "bin/materials-attachment.pdf");
    v = school.lecture(v.id);
    REQUIRE(v.attachmentName == "materials-attachment.pdf" && !v.attachment.empty());
    REQUIRE(readFile("data/" + v.attachment) == readFile("bin/materials-attachment.pdf"));
    writeFile("bin/materials-bad.pdf", "Not a PDF");
    REQUIRE(fails([&] { school.saveLecture(v, "bin/materials-bad.pdf"); }));
    auto graph = store.skillGraph();
    SkillGraph checked(graph);
    REQUIRE(htmlText("<p>A&nbsp; B<sup>2</sup></p><script>hidden</script><p>&#8722;5</p>") ==
            "A B^(2)\n−5");
    REQUIRE(fails([] { examResourceUrl("https://math-oge.sdamgia.ru.attacker.test/a"); }));
    REQUIRE(fails([] { examResourceUrl("http://math-oge.sdamgia.ru/a"); }));
    REQUIRE(fails([] { examResourceUrl("https://127.0.0.1/a"); }));
    auto svg = std::string(
        "<svg xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink' "
        "width='20' height='20'><defs><path id='p' d='M0 0 L10 0 L10 10 Z'/><use id='alias' "
        "xlink:href='#p'/></defs><use x='2' y='2' xlink:href='#alias'/></svg>");
    auto raster = svgToPng(svg);
    REQUIRE(raster.compare(0, 8, "\211PNG\r\n\032\n") == 0);
    REQUIRE(fails([] { svgToPng("<svg><image href='file:///secret'/></svg>"); }));
    ExamCategory category = {9, 43, "Квадратные уравнения", "oge_quadratic"};
    auto html = card();
    auto at = html.find("05.</div>");
    html.replace(at, 3, "05");
    auto parsed = parseSdamgia(html, category);
    REQUIRE(parsed.size() == 1);
    REQUIRE(parsed[0].task.parts[0].correctAnswer == "05");
    REQUIRE(parsed[0].task.description.find("Общее условие") != std::string::npos);
    REQUIRE(parsed[0].task.parts[0].prompt.find("НЕ ПОКАЗЫВАТЬ") == std::string::npos);
    REQUIRE(parsed[0].task.parts[0].prompt.find("[Рисунок 1]") != std::string::npos);
    REQUIRE(parseSdamgia(card("неизвестно"), category).empty());
    REQUIRE(parseSdamgia(html, {8, 54, "Степени", "oge_expressions"}).empty());
    auto triangleCard = html;
    triangleCard.replace(triangleCard.find("Тип 9"), std::string("Тип 9").size(), "Тип 16");
    triangleCard.replace(triangleCard.find("Найдите x: x − 5 = 0."),
                         std::string("Найдите x: x − 5 = 0.").size(),
                         "В прямоугольном треугольнике найдите площадь.");
    auto triangle = parseSdamgia(triangleCard, {16, 38, "Окружность", "oge_incircle"});
    REQUIRE(triangle.size() == 1 && triangle[0].task.parts[0].skills[0] == "oge_triangle_area");
    auto tableCard = html;
    tableCard.replace(tableCard.find("Тип 9"), std::string("Тип 9").size(), "Тип 1");
    tableCard.replace(tableCard.find("Найдите x: x − 5 = 0."),
                      std::string("Найдите x: x − 5 = 0.").size(),
                      "Установите соответствие между объемами и номерами печей.");
    auto table = parseSdamgia(tableCard, {1, 136, "Печи", "oge_practical"});
    REQUIRE(table.size() == 1 && table[0].task.parts[0].skills[0] == "oge_tables");
    ExamService exams(store);
    auto downloader = [svg](const std::string &) { return svg; };
    auto first = exams.importTask(parsed[0], downloader);
    REQUIRE(store.task(first).published);
    REQUIRE(store.examTasks(9, 43).size() == 1);
    REQUIRE(exams.importTask(parsed[0], downloader) == first);
    REQUIRE(store.examTasks(9, 43).size() == 1);
    REQUIRE(exams.load(43, 1, false).tasks == std::vector<std::string>({first}));
    REQUIRE(exams.load(43, 1, false, {}, {first}).tasks.empty());
    auto resource = "data/" + store.task(first).resources[0];
    writeFile(resource, "broken image");
    REQUIRE(exams.load(43, 1, false).tasks.empty());
    REQUIRE(exams.importTask(parsed[0], downloader) == first);
    REQUIRE(exams.load(43, 1, false).tasks.size() == 1);
    REQUIRE(fails([&] { exams.load(43, 0, false); }));
    auto changed = parsed[0];
    changed.task.parts[0].correctAnswer = "6";
    changed.metadata.fingerprint = "new-answer";
    auto second = exams.importTask(changed, downloader);
    REQUIRE(first != second);
    REQUIRE(store.task(first).parts[0].correctAnswer == "05");
    auto student = school.createProfile("ОГЭ: маршрут");
    SessionSettings settings;
    settings.training = true;
    settings.taskLimit = 2;
    school.start(student, settings, {first, second}, "teacher", "ОГЭ");
    store.archive(second);
    auto current = school.next();
    REQUIRE(current.task.id == first);
    auto lectures = school.recommendedLectures(current.attempt.id);
    REQUIRE(!lectures.empty());
    school.readLecture(lectures[0].id, current.attempt.id);
    school.readLecture(lectures[0].id, current.attempt.id);
    REQUIRE(store.helpCount(current.attempt.id) == 1);
    AnswerPayload value;
    value.text = "5";
    REQUIRE(school.submit(current.attempt.id, "answer", value, true).status ==
            CheckStatus::Incorrect);
    REQUIRE(school.canSupport(current.attempt.id));
    auto origin = current.task.id;
    current = school.startSupport(current.attempt.id);
    bool returned = false;
    int steps = 0;
    while (current.available && current.task.id != second && steps++ < 40) {
        if (current.task.id == origin)
            returned = true;
        value.text = current.task.parts[0].correctAnswer;
        REQUIRE(!value.text.empty());
        REQUIRE(school.submit(current.attempt.id, "answer", value, true).status ==
                CheckStatus::Correct);
        school.finish(current.attempt.id);
        current = school.next();
    }
    REQUIRE(returned && current.available && current.task.id == second && steps < 40);
    value.text = "6";
    school.submit(current.attempt.id, "answer", value, true);
    school.finish(current.attempt.id);
    REQUIRE(!school.next().available);
    auto progress = school.progress(student);
    REQUIRE(progress.score <= progress.maximumScore);
    auto before = school.snapshot(student);
    store.replayModels();
    auto after = school.snapshot(student);
    for (auto &entry : before.skills) {
        REQUIRE(entry.second.mastery == after.skills[entry.first].mastery);
        REQUIRE(entry.second.rating == after.skills[entry.first].rating);
    }
    SessionSettings assessment;
    assessment.helpAllowed = false;
    school.start(student, assessment, {first});
    current = school.next();
    REQUIRE(school.recommendedLectures(current.attempt.id).empty());
    REQUIRE(fails([&] { school.readLecture(lectures[0].id, current.attempt.id); }));
    store.logoutTeacher();
    REQUIRE(fails([&] { school.lectures(true); }));
    REQUIRE(fails([&] { school.saveLecture(v); }));
    auto started = std::chrono::steady_clock::now();
    SessionSettings adaptive;
    adaptive.mode = SessionMode::Dynamic;
    adaptive.targetSkills = {"oge_linear", "oge_quadratic", "oge_fractions"};
    adaptive.taskLimit = 10;
    for (int i = 0; i < 20; ++i) {
        auto profile = school.createProfile("Подбор " + std::to_string(i));
        school.start(profile, adaptive, {});
        REQUIRE(school.next().available);
    }
    auto selectionMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now() - started)
                           .count();
    started = std::chrono::steady_clock::now();
    for (int i = 0; i < 20; ++i)
        REQUIRE(exams.load(43, 1, false).tasks.size() == 1);
    auto cacheMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now() - started)
                       .count();
    std::cout << "Course bank: " << store.publishedTasks().size()
              << " tasks; 20 selections: " << selectionMs << " ms; 20 offline loads: " << cacheMs
              << " ms\n";
}
