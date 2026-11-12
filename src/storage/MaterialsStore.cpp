#include "Store.h"
#include "io/Json.h"
#include <set>
namespace exam {
void Store::seedLecture(const Lecture &v) {
    if (v.id.compare(0, 7, "course-") != 0)
        throw std::runtime_error("Неверный код учебного материала");
    Transaction tx(db);
    if (db.query("SELECT 1 FROM lectures WHERE id=?", {v.id}).empty()) {
        db.query("INSERT INTO lectures(id,title,body,published) VALUES(?,?,?,1)",
                 {v.id, v.title, v.body});
        for (const auto &s : v.skills)
            db.query("INSERT INTO lecture_skills VALUES(?,?)", {v.id, s});
    }
    tx.commit();
}
bool Store::hasTask(const std::string &id) {
    return !db.query("SELECT 1 FROM tasks WHERE id=?", {id}).empty();
}
std::vector<Lecture> Store::lectures(bool drafts) {
    if (drafts)
        requireTeacher();
    std::vector<Lecture> result;
    for (const auto &r :
         db.query("SELECT id,title,body,attachment,attachment_name,published,archived,revision "
                  "FROM lectures WHERE archived=0" +
                  std::string(drafts ? "" : " AND published=1") + " ORDER BY title,id")) {
        Lecture v;
        v.id = r[0];
        v.title = r[1];
        v.body = r[2];
        v.attachment = r[3];
        v.attachmentName = r[4];
        v.published = r[5] == "1";
        v.archived = r[6] == "1";
        v.revision = std::stoi(r[7]);
        for (const auto &s :
             db.query("SELECT skill_id FROM lecture_skills WHERE lecture_id=? ORDER BY skill_id",
                      {v.id}))
            v.skills.push_back(s[0]);
        result.push_back(v);
    }
    return result;
}
Lecture Store::lecture(const std::string &id, bool drafts) {
    for (const auto &v : lectures(drafts))
        if (v.id == id)
            return v;
    throw std::runtime_error("Лекция недоступна");
}
void Store::saveLecture(const Lecture &v) {
    requireTeacher();
    if (v.id.empty() || v.id.size() > 100 ||
        v.title.find_first_not_of(" \r\n\t") == std::string::npos || v.title.size() > 240 ||
        v.body.size() > 60000 || (v.body.empty() && v.attachment.empty()) || !validUtf8(v.title) ||
        !validUtf8(v.body) || v.body.find('\0') != std::string::npos ||
        v.title.find('\0') != std::string::npos || v.skills.size() > 100)
        throw std::runtime_error(
            "Заполните название и текст или прикрепите файл; текст — до 60 КБ");
    if (v.published && v.skills.empty())
        throw std::runtime_error("Свяжите лекцию хотя бы с одним навыком");
    Transaction tx(db);
    auto old = db.query("SELECT revision FROM lectures WHERE id=?", {v.id});
    if ((!old.empty() && std::stoi(old[0][0]) != v.revision) || (old.empty() && v.revision != 0))
        throw std::runtime_error("Лекция уже изменилась. Откройте её заново");
    db.query("INSERT INTO lectures(id,title,body,attachment,attachment_name,published,archived) "
             "VALUES(?,?,?,?,?,?,?) "
             "ON CONFLICT(id) DO UPDATE SET "
             "title=excluded.title,body=excluded.body,attachment=excluded.attachment,"
             "attachment_name=excluded.attachment_name,published=excluded.published,archived="
             "excluded.archived,"
             "revision=lectures.revision+1,updated_at=CURRENT_TIMESTAMP",
             {v.id, v.title, v.body, v.attachment, v.attachmentName, std::to_string(v.published),
              std::to_string(v.archived)});
    db.query("DELETE FROM lecture_skills WHERE lecture_id=?", {v.id});
    std::set<std::string> unique;
    for (const auto &s : v.skills) {
        if (!unique.insert(s).second)
            throw std::runtime_error("Навык повторяется");
        db.query("INSERT INTO lecture_skills VALUES(?,?)", {v.id, s});
    }
    tx.commit();
}
std::vector<ExamTask> Store::examTasks(int number, int category) {
    std::vector<ExamTask> result;
    for (const auto &r : db.query(
             "SELECT "
             "task_id,exam,subject,year,number,category,source,external_id,source_url,fingerprint "
             "FROM exam_tasks WHERE exam='oge' AND subject='math' AND year=2026 "
             "AND (?='0' OR number=?) AND (?='0' OR category=?) ORDER BY rowid DESC",
             {std::to_string(number), std::to_string(number), std::to_string(category),
              std::to_string(category)})) {
        ExamTask e;
        e.taskId = r[0];
        e.exam = r[1];
        e.subject = r[2];
        e.year = std::stoi(r[3]);
        e.number = std::stoi(r[4]);
        e.category = std::stoi(r[5]);
        e.source = r[6];
        e.externalId = r[7];
        e.sourceUrl = r[8];
        e.fingerprint = r[9];
        result.push_back(e);
    }
    return result;
}
void Store::saveExamTask(const ExamTask &e) {
    db.query("INSERT OR IGNORE INTO "
             "exam_tasks(task_id,exam,subject,year,number,category,source,external_id,source_url,"
             "fingerprint) "
             "VALUES(?,?,?,?,?,?,?,?,?,?)",
             {e.taskId, e.exam, e.subject, std::to_string(e.year), std::to_string(e.number),
              std::to_string(e.category), e.source, e.externalId, e.sourceUrl, e.fingerprint});
}
} // namespace exam
