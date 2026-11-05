#include "SchoolService.h"
#include "generation/Generator.h"
#include "training/TrainingService.h"
#include "io/Files.h"
#include "answers/AnswerPolicy.h"
#include <algorithm>
#include <cmath>
#include <set>
namespace exam {
std::vector<std::pair<std::string, std::string>> SchoolService::profiles() {
    return storage.profiles();
}
std::string SchoolService::createProfile(const std::string &name) {
    if (name.empty() || name.size() > 120)
        throw std::invalid_argument("Имя профиля: от 1 до 120 байтов");
    std::string id = newId("student");
    storage.saveProfile(id, name);
    return id;
}
std::vector<TaskVersion> SchoolService::tasks() {
    return storage.allTasks();
}
TaskVersion SchoolService::task(const std::string &id) {
    return storage.task(id);
}
std::vector<std::string> SchoolService::validate(const TaskVersion &task) const {
    std::vector<std::string> errors;
    auto containsNull = [](const std::string &value) {
        return value.find('\0') != std::string::npos;
    };
    if (containsNull(task.title) || containsNull(task.description) ||
        containsNull(task.commonFunction))
        errors.push_back("Нулевой символ в тексте задания");
    if (task.title.empty() || task.title.size() > 160)
        errors.push_back("Название: от 1 до 160 байтов");
    if (task.description.size() > 4096)
        errors.push_back("Общее условие длиннее 4096 байтов");
    if (task.parts.empty() || task.parts.size() > 100)
        errors.push_back("Нужно от 1 до 100 частей");
    if (!std::isfinite(task.difficulty) || task.difficulty < 0 || task.difficulty > 3000)
        errors.push_back("Сложность должна быть от 0 до 3000");
    std::set<std::string> ids, knownSkills;
    for (const SkillInfo &skill : storage.skills())
        knownSkills.insert(skill.id);
    for (const std::string &path : task.resources)
        if (!safeResourcePath(path))
            errors.push_back("Неверный путь ресурса");
    for (const TaskPart &part : task.parts) {
        std::string label = part.id + ": ";
        if (containsNull(part.prompt) || containsNull(part.expression))
            errors.push_back(label + "нулевой символ в условии");
        if (part.id.empty() || !ids.insert(part.id).second)
            errors.push_back("Идентификаторы частей должны быть уникальными");
        if (part.prompt.empty() || part.prompt.size() > 4096)
            errors.push_back(label + "нужно условие до 4096 байтов");
        if ((part.expression.empty() && part.kind != PartKind::Choice) ||
            part.expression.size() > 512)
            errors.push_back(label + "нужна формула до 512 байтов");
        if (part.maxScore < 1 || part.maxScore > 100)
            errors.push_back(label + "балл должен быть от 1 до 100");
        if (!std::isfinite(part.graphSnap) || part.graphSnap < 0.01 || part.graphSnap > 10 ||
            !std::isfinite(part.graphTolerance) || part.graphTolerance < 0.000001 ||
            part.graphTolerance > 0.1)
            errors.push_back(label + "привязка: 0.01–10; допуск: 0.000001–0.1");
        if (part.skills.empty())
            errors.push_back(label + "выберите хотя бы один навык");
        std::set<std::string> used;
        for (const std::string &skill : part.skills)
            if (!knownSkills.count(skill) || !used.insert(skill).second)
                errors.push_back(label + "неизвестный или повторный навык");
        try {
            if (!part.displayExpression.empty()) {
                if (part.displayExpression.size() > 512 || containsNull(part.displayExpression))
                    errors.push_back(label + "неверная формула условия");
                else if (part.displayExpression.find('=') != std::string::npos)
                    parseEquation(part.displayExpression);
                else
                    parseExpression(part.displayExpression);
            }
            std::string policyError = answerPolicyError(part);
            if (!policyError.empty())
                errors.push_back(label + policyError);
            if (part.kind == PartKind::Number || part.kind == PartKind::Choice ||
                part.kind == PartKind::Polynomial) {
                // Общая проверка формата выполнена выше.
            } else if (part.kind == PartKind::Expression)
                evaluateExact(parseExpression(part.expression));
            else if (part.kind == PartKind::Equation) {
                Equation equation = parseEquation(part.expression);
                Polynomial value = polynomial(equation.left) - polynomial(equation.right);
                solve(value);
                if (!task.commonFunction.empty() &&
                    !(value == polynomial(parseExpression(task.commonFunction))))
                    errors.push_back(label + "уравнение не соответствует общей функции");
            } else if (part.kind == PartKind::Line || part.kind == PartKind::Parabola) {
                Polynomial value = polynomial(parseExpression(part.expression));
                if ((part.kind == PartKind::Line && value.degree() > 1) ||
                    (part.kind == PartKind::Parabola && value.degree() != 2))
                    errors.push_back(label + "степень функции не соответствует типу графика");
                if (!task.commonFunction.empty() &&
                    !(value == polynomial(parseExpression(task.commonFunction))))
                    errors.push_back(label + "график не соответствует общей функции");
            } else
                errors.push_back(label + "неизвестный тип ответа");
        } catch (const MathError &error) {
            errors.push_back(label + error.what());
        }
    }
    return errors;
}
void SchoolService::saveDraft(const TaskVersion &task) {
    storage.saveDraft(task);
}
void SchoolService::publish(const std::string &id) {
    TaskVersion task = storage.task(id);
    std::vector<std::string> errors = validate(task);
    if (!errors.empty())
        throw std::runtime_error(errors.front());
    for (const auto &resource : task.resources)
        readFile(resourcePath("data", resource), 5 * 1024 * 1024);
    storage.publish(id);
}
void SchoolService::archive(const std::string &id) {
    storage.archive(id);
}
TaskVersion SchoolService::newVersion(const std::string &id) {
    TaskVersion copy = storage.task(id);
    copy.id = newId("task");
    copy.published = false;
    copy.archived = false;
    return copy;
}
std::vector<SkillInfo> SchoolService::skills() {
    return storage.skills();
}
std::vector<std::pair<std::string, std::string>> SchoolService::topics() {
    return storage.topics();
}
std::map<std::string, std::vector<std::string>> SchoolService::skillGraph() {
    return storage.skillGraph();
}
void SchoolService::saveTopic(const std::string &id, const std::string &name) {
    if (id.empty() || name.empty())
        throw std::invalid_argument("Заполните код и название темы");
    storage.saveTopic(id, name);
}
void SchoolService::saveSkill(const SkillInfo &skill, const std::vector<std::string> &parents) {
    if (skill.id.empty() || skill.name.empty())
        throw std::invalid_argument("Заполните код и название навыка");
    auto graph = storage.skillGraph();
    graph[skill.id] = parents;
    SkillGraph checked(graph);
    storage.saveSkillData(skill, parents);
}
void SchoolService::placeSkill(const std::string &id, Point point) {
    storage.saveSkillPosition(id, point);
}
StudentSnapshot SchoolService::snapshot(const std::string &profile) {
    return storage.snapshot(profile);
}
std::vector<HistoryEntry> SchoolService::history(const std::string &profile) {
    SessionService reader(storage, checker);
    std::vector<HistoryEntry> result;
    for (const SessionRecord &lesson : storage.studentSessions(profile))
        for (const AttemptRecord &attempt : storage.attempts(lesson.id)) {
            HistoryEntry entry;
            entry.attemptId = attempt.id;
            entry.taskId = attempt.taskId;
            const TaskVersion task = storage.task(attempt.taskId);
            entry.title = task.title;
            for (const auto &part : task.parts)
                entry.maximumScore += part.maxScore;
            entry.reason = attempt.selectionReason;
            entry.result = reader.result(attempt.id);
            entry.help = storage.helpCount(attempt.id);
            if (entry.result.visible)
                for (const auto &response : storage.responses(attempt.id))
                    if (response.result.status == CheckStatus::Incorrect)
                        entry.hadErrors = true;
            result.push_back(entry);
        }
    return result;
}
void SchoolService::prepareSession() {
    session.reset();
    adaptation.reset(new Adaptation(storage.modelParameters(), SkillGraph(storage.skillGraph())));
    session.reset(new SessionService(storage, checker, adaptation.get(), adaptation.get()));
}
SessionRecord SchoolService::latestLesson(const std::string &profile) {
    const auto lessons = storage.studentSessions(profile);
    return lessons.empty() ? SessionRecord() : lessons.front();
}
std::vector<AttemptRecord> SchoolService::lessonAttempts(const std::string &lesson) {
    return lesson.empty() ? std::vector<AttemptRecord>() : storage.attempts(lesson);
}
LessonProgress SchoolService::progress(const std::string &profile) {
    LessonProgress result;
    result.lesson = latestLesson(profile);
    if (result.lesson.id.empty())
        return result;
    bool fixed = result.lesson.settings.mode == SessionMode::Fixed;
    result.totalTasks = fixed ? std::min(result.lesson.settings.taskLimit,
                                         static_cast<int>(result.lesson.tasks.size()))
                              : result.lesson.settings.taskLimit;
    if (fixed)
        for (int i = 0; i < result.totalTasks; ++i)
            for (const auto &part : storage.task(result.lesson.tasks[i]).parts)
                result.maximumScore += part.maxScore;
    SessionService reader(storage, checker);
    for (const auto &attempt : storage.attempts(result.lesson.id)) {
        if (result.lesson.settings.training && !storage.trainingStep(attempt.id).stage.empty())
            ++result.totalTasks;
        if (!fixed)
            for (const auto &part : storage.task(attempt.taskId).parts)
                result.maximumScore += part.maxScore;
        if (!attempt.finished)
            continue;
        ++result.finishedTasks;
        const AttemptResult value = reader.result(attempt.id);
        if (!value.visible)
            result.hidden = true;
        else {
            result.score += value.score;
            result.provisional = result.provisional || value.provisional;
        }
    }
    // Не раскрываем даже частичную сумму до разрешённого момента проверки.
    if (result.hidden)
        result.score = 0;
    if (result.lesson.settings.training && result.lesson.closed)
        result.totalTasks = result.finishedTasks;
    return result;
}
void SchoolService::startTopics(const std::string &profile, const std::vector<std::string> &skills,
                                bool training, int count) {
    if (skills.empty())
        throw std::runtime_error("Выберите хотя бы одну тему или навык");
    if (count < 1 || count > 20)
        throw std::runtime_error("Число основных заданий: от 1 до 20");
    std::set<std::string> unique(skills.begin(), skills.end());
    auto graph = storage.skillGraph();
    for (const auto &skill : unique)
        if (!graph.count(skill))
            throw std::runtime_error("Неизвестный навык: " + skill);
    SessionSettings settings;
    settings.targetSkills.assign(unique.begin(), unique.end());
    settings.training = training;
    settings.mode = training ? SessionMode::Dynamic : SessionMode::Fixed;
    settings.feedback = training ? FeedbackPolicy::AfterStep : FeedbackPolicy::AfterSet;
    settings.taskLimit = count;
    auto candidates = TrainingService(storage).scopedTasks(settings.targetSkills);
    if (candidates.empty())
        throw std::runtime_error("По выбранным навыкам нет опубликованных заданий");
    if (!training && candidates.size() < static_cast<std::size_t>(count))
        throw std::runtime_error("В банке только " + std::to_string(candidates.size()) +
                                 " подходящих заданий. Уменьшите число заданий.");
    std::vector<std::string> tasks;
    if (!training) {
        // Набор определяется до старта и больше не зависит от ответов.
        Adaptation selector(storage.modelParameters(), SkillGraph(graph));
        for (int i = 0; i < count; ++i) {
            auto choice = selector.selectNext(storage.snapshot(profile), candidates, tasks);
            tasks.push_back(choice.task.id);
            candidates.erase(
                std::remove_if(candidates.begin(), candidates.end(),
                               [&](const TaskVersion &task) { return task.id == choice.task.id; }),
                candidates.end());
        }
    }
    start(profile, settings, tasks);
}
bool SchoolService::canSupport(const std::string &attempt) {
    return TrainingService(storage).canSupport(attempt);
}
std::vector<std::string> SchoolService::reviewSkills(const std::string &profile) {
    const SessionRecord lesson = latestLesson(profile);
    if (lesson.id.empty() || !lesson.closed)
        return {};
    std::set<std::string> skills;
    for (const auto &attempt : storage.attempts(lesson.id)) {
        std::set<std::string> wrongParts;
        for (const auto &response : storage.responses(attempt.id))
            if (response.result.status == CheckStatus::Incorrect)
                wrongParts.insert(response.partId);
        for (const auto &part : storage.task(attempt.taskId).parts)
            if (wrongParts.count(part.id))
                skills.insert(part.skills.begin(), part.skills.end());
    }
    return std::vector<std::string>(skills.begin(), skills.end());
}
NextTask SchoolService::startSupport(const std::string &attempt) {
    if (!canSupport(attempt))
        throw std::runtime_error("Разбор сейчас недоступен");
    session->finishAttempt(attempt);
    TrainingService(storage).beginSupport(attempt);
    return next();
}
void SchoolService::start(const std::string &profile, const SessionSettings &settings,
                          const std::vector<std::string> &tasks) {
    for (const SessionRecord &old : storage.studentSessions(profile))
        if (!old.closed) {
            for (const AttemptRecord &attempt : storage.attempts(old.id))
                if (!attempt.finished)
                    throw std::runtime_error("Сначала продолжите незавершённое занятие");
            storage.closeSession(old.id);
        }
    prepareSession();
    SessionRecord lesson;
    lesson.id = newId("session");
    lesson.studentId = profile;
    lesson.settings = settings;
    if (settings.mode == SessionMode::Fixed)
        lesson.tasks = tasks;
    session->start(lesson);
    activeSession = lesson.id;
}
bool SchoolService::resume(const std::string &profile) {
    for (const SessionRecord &lesson : storage.studentSessions(profile))
        if (!lesson.closed) {
            prepareSession();
            activeSession = lesson.id;
            return true;
        }
    return false;
}
NextTask SchoolService::next() {
    if (!session)
        throw std::runtime_error("Занятие не начато");
    return session->next(activeSession);
}
CheckResult SchoolService::submit(const std::string &attempt, const std::string &part,
                                  const AnswerPayload &answer, bool finalAnswer,
                                  const std::string &operation, const std::string &operand) {
    if (!session)
        throw std::runtime_error("Занятие не начато");
    ResponseRecord response;
    response.requestId = newId("response");
    response.attemptId = attempt;
    response.partId = part;
    response.answer = answer;
    response.finalAnswer = finalAnswer;
    response.operation = operation;
    response.operand = operand;
    return session->submitResponse(response);
}
std::vector<ResponseRecord> SchoolService::responses(const std::string &attempt) {
    SessionService reader(storage, checker);
    return reader.history(attempt);
}
void SchoolService::help(const std::string &attempt, const std::string &part) {
    session->requestHelp(newId("help"), attempt, part, "hint");
}
AttemptResult SchoolService::finish(const std::string &attempt) {
    return session->finishAttempt(attempt);
}
void SchoolService::closeLesson() {
    if (!session)
        return;
    storage.closeSession(activeSession);
}
void SchoolService::addSampleBank() {
    storage.saveTopic("algebra", "Алгебра");
    const char *ids[] = {"fractions", "linear", "quadratic", "line_graph", "parabola_graph"};
    const char *names[] = {"Дроби", "Линейные уравнения", "Квадратные уравнения", "Прямая",
                           "Парабола"};
    for (int i = 0; i < 5; ++i)
        storage.saveSkill(ids[i], "algebra", names[i]);
    storage.savePrerequisite("quadratic", "linear");
    storage.savePrerequisite("line_graph", "linear");
    storage.savePrerequisite("parabola_graph", "quadratic");
    Generator generator;
    std::set<std::string> existing;
    for (const TaskVersion &task : storage.allTasks())
        existing.insert(task.id);
    for (int family = 0; family < 5; ++family)
        for (int example = 0; example < 3; ++example) {
            GenerationRequest request;
            request.id = "sample-" + std::to_string(family) + "-" + std::to_string(example);
            if (existing.count(request.id))
                continue;
            request.family = static_cast<Family>(family);
            request.seed = 12 + example * 31 + family;
            TaskVersion task = generator.generateTask(request);
            task.title = "Учебный пример: " + task.title;
            storage.saveDraft(task);
            publish(task.id);
        }
}
} // namespace exam
