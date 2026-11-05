#include "Store.h"
#include "core/GraphCodec.h"
#include <ctime>
#include <stdexcept>
namespace exam {
namespace {
void check(bool value, const std::string &message) {
    if (!value)
        throw std::runtime_error(message);
}
long long clockTime(long long now) {
    return now < 0 ? static_cast<long long>(std::time(0)) : now;
}
} // namespace
void Store::saveTemplate(const LessonTemplate &value) {
    const SessionSettings &s = value.settings;
    check(!value.id.empty() && !value.name.empty(), "Укажите название шаблона");
    check(s.taskLimit > 0 && s.taskLimit <= 1000 && s.timeLimitSeconds >= 0 &&
              s.timeLimitSeconds <= 86400 && static_cast<int>(s.resumePolicy) >= 0 &&
              static_cast<int>(s.resumePolicy) <= 2,
          "Неверные ограничения шаблона");
    check(s.mode != SessionMode::Fixed ||
              (!value.tasks.empty() && value.tasks.size() <= static_cast<std::size_t>(s.taskLimit)),
          "Пустой или слишком большой набор");
    Transaction transaction(db);
    db.query(
        "INSERT INTO lesson_templates VALUES(?,?,?,?,?,?,?,?,?) ON CONFLICT(id) DO UPDATE SET "
        "name=excluded.name,mode=excluded.mode,feedback=excluded.feedback,guided=excluded.guided,"
        "training=excluded.training,task_limit=excluded.task_limit,resume_policy=excluded.resume_"
        "policy,"
        "time_limit=excluded.time_limit",
        {value.id, value.name, std::to_string(int(s.mode)), std::to_string(int(s.feedback)),
         std::to_string(s.guided), std::to_string(s.training), std::to_string(s.taskLimit),
         std::to_string(int(s.resumePolicy)), std::to_string(s.timeLimitSeconds)});
    db.query("DELETE FROM template_tasks WHERE template_id=?", {value.id});
    db.query("DELETE FROM template_skills WHERE template_id=?", {value.id});
    for (std::size_t i = 0; i < value.tasks.size(); ++i) {
        TaskVersion taskValue = task(value.tasks[i]);
        check(taskValue.published && !taskValue.archived, "Задание недоступно");
        db.query("INSERT INTO template_tasks VALUES(?,?,?)",
                 {value.id, std::to_string(i), value.tasks[i]});
    }
    for (const auto &skill : s.targetSkills)
        db.query("INSERT INTO template_skills VALUES(?,?)", {value.id, skill});
    transaction.commit();
}
LessonTemplate Store::lessonTemplate(const std::string &id) {
    auto rows =
        db.query("SELECT name,mode,feedback,guided,training,task_limit,resume_policy,time_limit "
                 "FROM lesson_templates WHERE id=?",
                 {id});
    check(!rows.empty(), "Шаблон не найден");
    const Row &r = rows[0];
    LessonTemplate result;
    result.id = id;
    result.name = r[0];
    result.settings.mode = static_cast<SessionMode>(std::stoi(r[1]));
    result.settings.feedback = static_cast<FeedbackPolicy>(std::stoi(r[2]));
    result.settings.guided = r[3] == "1";
    result.settings.training = r[4] == "1";
    result.settings.taskLimit = std::stoi(r[5]);
    result.settings.resumePolicy = static_cast<ResumePolicy>(std::stoi(r[6]));
    result.settings.timeLimitSeconds = std::stoi(r[7]);
    for (const Row &row :
         db.query("SELECT task_id FROM template_tasks WHERE template_id=? ORDER BY position", {id}))
        result.tasks.push_back(row[0]);
    for (const Row &row : db.query(
             "SELECT skill_id FROM template_skills WHERE template_id=? ORDER BY skill_id", {id}))
        result.settings.targetSkills.push_back(row[0]);
    return result;
}
void Store::startTemplate(const std::string &templateId, const std::string &sessionId,
                          const std::string &student) {
    LessonTemplate source = lessonTemplate(templateId);
    SessionRecord value;
    value.id = sessionId;
    value.studentId = student;
    value.settings = source.settings;
    value.tasks = source.tasks;
    createSession(value);
}
SessionTime Store::sessionTime(const std::string &id, long long now) {
    now = clockTime(now);
    SessionRecord lesson = session(id);
    SessionTime result;
    if (lesson.closed) {
        result.allowed = false;
        result.message = "Занятие завершено";
        return result;
    }
    Transaction transaction(db);
    db.query(
        "INSERT OR IGNORE INTO session_clocks(session_id,started,last_seen,owner) VALUES(?,?,?,?)",
        {id, std::to_string(now), std::to_string(now), runtimeId});
    Row row = db.query("SELECT started,last_seen,elapsed,paused,owner,stopped FROM session_clocks "
                       "WHERE session_id=?",
                       {id})[0];
    long long started = std::stoll(row[0]), last = std::stoll(row[1]), elapsed = std::stoll(row[2]);
    result.paused = row[3] == "1";
    std::string stopped = row[5];
    if (stopped.empty() && now < last)
        stopped = "Системные часы переведены назад. Занятие остановлено; ответы сохранены";
    if (stopped.empty() && lesson.settings.resumePolicy == ResumePolicy::NoResume &&
        row[4] != runtimeId)
        stopped = "Продолжение после закрытия программы запрещено условиями занятия";
    if (stopped.empty()) {
        // При аварийном закрытии незаявленная пауза не освобождает от ограничения времени.
        if (lesson.settings.resumePolicy == ResumePolicy::Deadline)
            elapsed = now - started;
        else if (!result.paused)
            elapsed += now - last;
        if (lesson.settings.timeLimitSeconds > 0) {
            result.remainingSeconds = std::max(0LL, lesson.settings.timeLimitSeconds - elapsed);
            if (!result.remainingSeconds)
                stopped = "Время занятия истекло";
        }
    }
    db.query(
        "UPDATE session_clocks SET last_seen=?,elapsed=?,owner=?,stopped=? WHERE session_id=?",
        {std::to_string(std::max(now, last)), std::to_string(elapsed), runtimeId, stopped, id});
    transaction.commit();
    result.allowed = stopped.empty() && !result.paused;
    if (!stopped.empty())
        result.paused = false;
    result.message = stopped.empty() ? (result.paused ? "Занятие на паузе" : "") : stopped;
    return result;
}
void Store::pauseSession(const std::string &id, long long now) {
    check(session(id).settings.resumePolicy == ResumePolicy::ActiveTime,
          "В этом занятии пауза запрещена");
    SessionTime time = sessionTime(id, now);
    check(time.allowed || time.paused, time.message);
    db.query("UPDATE session_clocks SET paused=1 WHERE session_id=? AND stopped=''", {id});
}
void Store::resumeSession(const std::string &id, long long now) {
    SessionTime time = sessionTime(id, now);
    check(time.allowed || time.paused, time.message);
    auto rows = db.query("SELECT stopped FROM session_clocks WHERE session_id=?", {id});
    check(rows[0][0].empty(), rows[0][0]);
    db.query("UPDATE session_clocks SET paused=0 WHERE session_id=?", {id});
}
void Store::requireSessionAccess(const std::string &id) {
    SessionTime time = sessionTime(id);
    check(time.allowed, time.message);
}
void Store::saveAnswerDraft(const std::string &id, const std::string &part,
                            const AnswerPayload &answer) {
    AttemptRecord value = attempt(id);
    check(!value.finished && answer.text.size() <= 512, "Нельзя сохранить этот черновик");
    requireSessionAccess(value.sessionId);
    bool found = false;
    for (const auto &item : task(value.taskId).parts)
        if (item.id == part)
            found = true;
    check(found, "Часть не найдена");
    db.query("INSERT INTO answer_drafts VALUES(?,?,?,?) ON CONFLICT(attempt_id,part_id) "
             "DO UPDATE SET text=excluded.text,graph=excluded.graph",
             {id, part, answer.text, encodeGraph(answer.graph)});
}
std::map<std::string, AnswerPayload> Store::answerDrafts(const std::string &id) {
    std::map<std::string, AnswerPayload> result;
    for (const Row &row :
         db.query("SELECT part_id,text,graph FROM answer_drafts WHERE attempt_id=?", {id})) {
        result[row[0]].text = row[1];
        result[row[0]].graph = decodeGraph(row[2]);
    }
    return result;
}
} // namespace exam
