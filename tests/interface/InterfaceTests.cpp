#include "../Test.h"
#include "../Fixture.h"
#include "application/SchoolService.h"
#include "io/packages/PackageService.h"
#include "io/Files.h"
#include "core/GraphCodec.h"
#include <cmath>
#include <limits>
#include <functional>
using namespace exam;
namespace {
void rejects(const std::function<void()> &action) {
    bool rejected = false;
    try {
        action();
    } catch (const std::exception &) {
        rejected = true;
    }
    REQUIRE(rejected);
}
TaskVersion graphTask() {
    TaskVersion task;
    task.id = "graph-task";
    task.title = "Корни и график";
    task.description = "Общее условие";
    task.commonFunction = "x^2-4";
    TaskPart root;
    root.id = "roots";
    root.prompt = "Найдите корни";
    root.expression = "x^2-4=0";
    root.skills = {"quadratic"};
    task.parts.push_back(root);
    root.id = "graph";
    root.kind = PartKind::Parabola;
    root.prompt = "Постройте график";
    root.expression = "x^2-4";
    root.skills = {"parabola_graph"};
    root.maxScore = 2;
    task.parts.push_back(root);
    return task;
}
} // namespace
void testGeometry() {
    GeometryChecker checker;
    TaskPart line;
    line.kind = PartKind::Line;
    line.expression = "2*x+1";
    GraphAnswer answer;
    REQUIRE(checker.check(line, answer).status == CheckStatus::Incomplete);
    answer.points = {Point(0, 1), Point(2, 5)};
    REQUIRE(checker.check(line, answer).status == CheckStatus::Correct);
    GraphAnswer restored = decodeGraph(encodeGraph(answer));
    REQUIRE(checker.check(line, restored).status == CheckStatus::Correct);
    Coordinates coordinates(800, 450);
    Point original(2.5, -3.5), pixel = coordinates.toScreen(original);
    Point anchor = coordinates.toWorld(Point(130, 220));
    coordinates.zoom(2, Point(130, 220));
    REQUIRE(std::abs(coordinates.toWorld(Point(130, 220)).x - anchor.x) < 1e-9);
    coordinates.pan(40, -20);
    pixel = coordinates.toScreen(original);
    Point recovered = coordinates.toWorld(pixel);
    REQUIRE(std::abs(recovered.x - original.x) < 1e-9 && std::abs(recovered.y - original.y) < 1e-9);
    REQUIRE(checker.check(line, answer).status == CheckStatus::Correct);
    answer.points[1] = Point(0, 2);
    REQUIRE(checker.check(line, answer).status == CheckStatus::InvalidInput);
    answer.points = {Point(0, 1), Point(2, 6)};
    REQUIRE(checker.check(line, answer).status == CheckStatus::Incorrect);
    answer.points[1].x = std::numeric_limits<double>::infinity();
    REQUIRE(checker.check(line, answer).status == CheckStatus::InvalidInput);
    TaskPart parabola;
    parabola.kind = PartKind::Parabola;
    parabola.expression = "x^2-4";
    answer = GraphAnswer();
    answer.coefficients = {1, 0, -4};
    REQUIRE(checker.check(parabola, answer).status == CheckStatus::Correct);
    answer.coefficients[0] = 0;
    REQUIRE(checker.check(parabola, answer).status == CheckStatus::InvalidInput);
    REQUIRE(Coordinates::snap(1.23, 0.5) == 1.0);
}
void testJsonPackages() {
    Json value =
        parseJson("{\"text\":\"\\u0410\\u043d\\u0430\\u0441\\u0442\\u0430\\u0441\\u0438\\u044f\","
                  "\"emoji\":\"\\ud83d\\ude00\",\"array\":[true,false,null,-1.2e2]}");
    REQUIRE(value.field("text").text() == "Анастасия");
    REQUIRE(parseJson(writeJson(value)).field("emoji").text() == value.field("emoji").text());
    for (const std::string &invalid :
         {"[1,]", "{\"x\":1,}", "{\"x\":1,\"x\":2}", "01", "1.", "1e999", "true false",
          "\"\\ud800\"", "\"\\udc00\"", "\"\\q\""})
        rejects([invalid] { parseJson(invalid); });
    rejects([] { parseJson(std::string(34, '[') + "0" + std::string(34, ']')); });
    rejects([] { parseJson(std::string(2 * 1024 * 1024 + 1, ' ')); });
    rejects([] { parseJson(std::string("\"") + char(0xc0) + char(0xaf) + "\""); });
    rejects([] { parseJson("1.5").integer(); });
    REQUIRE(safeResourcePath("images/graph.png"));
    for (const std::string &invalid :
         {"../graph.png", "/graph.png", "C:/graph.png", "images\\graph.png", "con.png",
          "a/../../b.jpg", "image.exe", "x.png:other"})
        REQUIRE(!safeResourcePath(invalid));
    std::string folder = "bin/package-test-" + newId("run");
    makeDirectories(folder + "/source/images");
    makeDirectories(folder + "/export");
    makeDirectories(folder + "/target");
    Store first(":memory:");
    prepareBank(first);
    SchoolService school(first);
    TaskVersion task = graphTask();
    task.resources = {"images/picture.png"};
    std::string image = "resource bytes";
    writeFile(folder + "/source/images/picture.png", image);
    school.saveDraft(task);
    PackageService exporter(school, folder + "/source");
    exporter.exportPackage(folder + "/export/tasks.json", {task.id});
    Store second(":memory:");
    prepareBank(second);
    SchoolService target(second);
    PackageService importer(target, folder + "/target");
    PackagePreview preview = importer.inspect(folder + "/export/tasks.json");
    REQUIRE(preview.items.size() == 1 && preview.items[0].error.empty());
    rejects([&] { importer.importPackage(preview, {}); });
    auto report = importer.importPackage(
        preview, {{"quadratic", "quadratic"}, {"parabola_graph", "parabola_graph"}});
    REQUIRE(report.size() == 1 && report[0].imported);
    TaskVersion copy = target.task("school.graph-task");
    REQUIRE(!copy.published && copy.parts.size() == 2 && copy.description == task.description &&
            copy.commonFunction == task.commonFunction);
    REQUIRE(readFile(folder + "/target/" + copy.resources[0]) == image);
    REQUIRE(copy.parts[1].kind == PartKind::Parabola && copy.parts[1].maxScore == 2);
    REQUIRE(!importer
                 .importPackage(
                     preview, {{"quadratic", "quadratic"}, {"parabola_graph", "parabola_graph"}})[0]
                 .imported);
    REQUIRE(target.tasks().size() == 1);
    Json bad = parseJson(readFile(folder + "/export/tasks.json"));
    bad.object["tasks"].array[0].object["resources"].array[0] = Json(std::string("../bad.png"));
    writeFile(folder + "/export/bad.json", writeJson(bad));
    REQUIRE(!importer.inspect(folder + "/export/bad.json").items[0].error.empty());
    bad.object["tasks"].array[0].object["resources"] = Json::makeArray();
    bad.object["tasks"].array[0].object["parts"].array[0].object["kind"] = Json(99.0);
    writeFile(folder + "/export/bad.json", writeJson(bad));
    REQUIRE(!importer.inspect(folder + "/export/bad.json").items[0].error.empty());
    REQUIRE(target.history("student").empty());
}
void testSchool() {
    {
        Store demoStore(":memory:");
        SchoolService demo(demoStore);
        demo.addStarterContent();
        REQUIRE(demo.tasks().size() == 5);
        REQUIRE(demo.profiles().size() == 1);
        REQUIRE(demo.history("demo-student").size() == 2);
        REQUIRE(demo.history("demo-student")[0].result.visible);
        auto snapshot = demo.snapshot("demo-student");
        REQUIRE(snapshot.skills["fractions"].known && snapshot.skills["linear"].known);
        demo.addStarterContent();
        REQUIRE(demo.tasks().size() == 5 && demo.history("demo-student").size() == 2);
        REQUIRE(demo.snapshot("demo-student").skills["linear"].observations ==
                snapshot.skills["linear"].observations);
    }
    Store store(":memory:");
    prepareBank(store);
    SchoolService school(store);
    TaskVersion task = graphTask();
    school.saveDraft(task);
    school.publish(task.id);
    TaskVersion published = school.task(task.id);
    REQUIRE(published.published);
    rejects([&] { school.saveDraft(published); });
    TaskVersion changed = school.newVersion(task.id);
    REQUIRE(changed.id != task.id && !changed.published);
    changed.parts[0].expression = "x^2-5=0";
    school.saveDraft(changed);
    rejects([&] { school.publish(changed.id); });
    REQUIRE(school.task(task.id).parts[0].expression == "x^2-4=0");
    SkillInfo skill;
    skill.id = "extra";
    skill.topicId = "algebra";
    skill.name = "Дополнительный навык";
    school.saveSkill(skill, {"linear"});
    SkillInfo linear;
    linear.id = "linear";
    linear.topicId = "algebra";
    linear.name = "Линейные уравнения";
    rejects([&] { school.saveSkill(linear, {"extra"}); });
    REQUIRE(school.skillGraph()["linear"].empty());
    REQUIRE(!school.snapshot("student").skills["extra"].known);
    school.placeSkill("extra", Point(50, 80));
    bool placed = false;
    for (const auto &item : school.skills())
        if (item.id == "extra")
            placed = item.placed && item.position.x == 50;
    REQUIRE(placed);
    SessionSettings settings;
    settings.feedback = FeedbackPolicy::AfterSet;
    settings.taskLimit = 1;
    school.start("student", settings, {task.id});
    NextTask next = school.next();
    REQUIRE(next.available);
    AnswerPayload roots;
    roots.text = "-2;2";
    REQUIRE(school.submit(next.attempt.id, "roots", roots, true).status == CheckStatus::Incomplete);
    AnswerPayload graph;
    graph.graph.coefficients = {1, 0, -4};
    school.submit(next.attempt.id, "graph", graph, true);
    school.help(next.attempt.id, "graph");
    for (const auto &response : school.responses(next.attempt.id))
        REQUIRE(response.result.status == CheckStatus::Incomplete);
    AttemptResult result = school.finish(next.attempt.id);
    REQUIRE(result.visible && !result.provisional && result.score == 3);
    REQUIRE(!school.next().available);
    REQUIRE(school.history("student")[0].help == 1);
    REQUIRE(school.responses(next.attempt.id)[1].answer.graph.coefficients.size() == 3);
    settings.mode = SessionMode::Dynamic;
    settings.feedback = FeedbackPolicy::AfterTask;
    school.start("student", settings, {});
    next = school.next();
    REQUIRE(next.available);
    school.submit(next.attempt.id, "roots", roots, true);
    school.submit(next.attempt.id, "graph", graph, true);
    REQUIRE(school.finish(next.attempt.id).score == 3);
    REQUIRE(!school.next().available);
    school.addSampleBank();
    auto count = school.tasks().size();
    school.addSampleBank();
    REQUIRE(school.tasks().size() == count);
}
