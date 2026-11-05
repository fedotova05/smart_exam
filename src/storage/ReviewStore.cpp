#include "Store.h"
#include "adaptation/Adaptation.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>
namespace exam {
namespace {
std::string precise(double value) {
    std::ostringstream result; result << std::setprecision(17) << value; return result.str();
}
bool hasText(const std::string &text) { return text.find_first_not_of(" \t\n\r") != std::string::npos; }
}
std::vector<ReviewVerdict> Store::reviewHistory(const std::string &id) {
    std::vector<ReviewVerdict> result;
    for (const Row &r : db.query("SELECT request_id,part_id,status,reviewer,reason FROM review_verdicts "
                                 "WHERE attempt_id=? ORDER BY sequence", {id})) {
        ReviewVerdict value; value.attemptId=id; value.requestId=r[0]; value.partId=r[1];
        value.status=static_cast<CheckStatus>(std::stoi(r[2])); value.reviewer=r[3]; value.reason=r[4];
        result.push_back(value);
    }
    return result;
}
void Store::reviewPart(const ReviewVerdict &value) {
    if (!hasText(value.requestId) || !hasText(value.reviewer) || !hasText(value.reason) ||
        value.reason.size()>2048 || value.reviewer.size()>120 ||
        (value.status != CheckStatus::Correct && value.status != CheckStatus::Incorrect &&
         value.status != CheckStatus::Unsupported))
        throw std::invalid_argument("Укажите автора, причину и вердикт: верно, неверно или требует проверки");
    auto old = db.query("SELECT attempt_id,part_id,status,reviewer,reason FROM review_verdicts WHERE request_id=?", {value.requestId});
    Row content={value.attemptId,value.partId,std::to_string(int(value.status)),value.reviewer,value.reason};
    if (!old.empty()) {
        if (old[0]!=content) throw std::runtime_error("Номер пересмотра уже использован с другим содержимым");
        return;
    }
    AttemptRecord a=attempt(value.attemptId);
    if (!a.finished) throw std::runtime_error("Сначала завершите попытку");
    bool found=false;
    for (const auto &part : task(a.taskId).parts) if (part.id==value.partId) found=true;
    if (!found) throw std::runtime_error("Часть не найдена");
    ReplayData data=calculateReplay(&value);
    Transaction transaction(db);
    saveReplay(data);
    db.query("INSERT INTO review_verdicts(request_id,attempt_id,part_id,status,reviewer,reason) VALUES(?,?,?,?,?,?)",
             {value.requestId,value.attemptId,value.partId,std::to_string(int(value.status)),value.reviewer,value.reason});
    transaction.commit();
}
void Store::replayModels(const std::function<void()> &checkpoint) {
    ReplayData data=calculateReplay(0,checkpoint);
    if (checkpoint) checkpoint();
    Transaction transaction(db); saveReplay(data); transaction.commit();
}
double Store::taskRating(const std::string &id) {
    auto rows=db.query("SELECT rating FROM task_ratings WHERE task_id=?", {id});
    return rows.empty() ? task(id).difficulty : std::stod(rows[0][0]);
}
Row Store::modelRevision() {
    return db.query("SELECT (SELECT COALESCE(MAX(sequence),0) FROM learning_events),"
                    "(SELECT COALESCE(MAX(sequence),0) FROM review_verdicts)")[0];
}
Store::ReplayData Store::calculateReplay(const ReviewVerdict *pending, const std::function<void()> &checkpoint) {
    ReplayData result; result.revision=modelRevision();
    // Один порядок событий для всех учеников: общая трудность связывает их рейтинги.
    auto &students=result.students;
    auto &difficulties=result.difficulties;
    SkillGraph graph(skillGraph());
    auto events=db.query("SELECT attempt_id,has_model FROM learning_events ORDER BY sequence");
    for (const Row &event : events) {
        if (checkpoint) checkpoint();
        AttemptRecord a=attempt(event[0]);
        TaskVersion value=task(a.taskId);
        auto answers=responses(a.id);
        std::map<std::string,ReviewVerdict> verdicts;
        for (const auto &verdict : reviewHistory(a.id)) verdicts[verdict.partId]=verdict;
        if (pending && pending->attemptId==a.id) verdicts[pending->partId]=*pending;
        std::vector<PartEvidence> evidence;
        int score=0; bool provisional=false;
        for (const TaskPart &part : value.parts) {
            PartEvidence item; item.skills=part.skills; item.usedHelp=usedHelp(a.id,part.id);
            std::string finalRequest;
            for (const auto &answer : answers)
                if (answer.partId==part.id && answer.finalAnswer) {
                    item.finalStatus=answer.result.status; finalRequest=answer.requestId;
                }
            for (const auto &answer : answers)
                if (answer.partId==part.id && answer.result.status==CheckStatus::Incorrect &&
                    !(verdicts.count(part.id) && answer.requestId==finalRequest)) item.hadError=true;
            if (verdicts.count(part.id)) item.finalStatus=verdicts[part.id].status;
            if (item.finalStatus==CheckStatus::Correct) score+=part.maxScore;
            else if (item.finalStatus!=CheckStatus::Incorrect) provisional=true;
            evidence.push_back(item);
        }
        // Попытки без подключённой модели сохраняют исходное поведение, включая тестовые записи.
        if (!verdicts.empty())
            result.grades[a.id]={score,provisional};
        if (event[1]!="1" || trainingStep(a.id).stage=="return") continue;
        const Row params=db.query("SELECT version,initial,learn,slip,guess,k FROM attempt_models WHERE attempt_id=?", {a.id})[0];
        ModelParameters p; p.version=params[0]; p.initial=std::stod(params[1]); p.learn=std::stod(params[2]);
        p.slip=std::stod(params[3]); p.guess=std::stod(params[4]); p.k=std::stod(params[5]);
        Adaptation model(p,graph);
        std::string student=session(a.sessionId).studentId;
        StudentSnapshot &snapshot=students[student]; snapshot.studentId=student; snapshot.modelVersion=p.version;
        if (!difficulties.count(value.id)) difficulties[value.id]=value.difficulty;
        double difficulty=difficulties[value.id], change=0; int count=0;
        auto observations=model.observe(snapshot,evidence,difficulty);
        for (const auto &o : observations) {
            result.observations.push_back({a.id,o});
            if (!o.eligible) continue;
            snapshot.skills[o.skill]=o.after;
            change+=o.after.rating-o.before.rating; ++count;
        }
        if (count) difficulties[value.id]=std::max(0.0,std::min(3000.0,difficulty-change/count));
    }
    if (result.revision!=modelRevision()) throw std::runtime_error("История изменилась; повторите пересчёт");
    return result;
}
void Store::saveReplay(const ReplayData &data) {
    if (data.revision!=modelRevision()) throw std::runtime_error("История изменилась; повторите пересчёт");
    for (const auto &entry : data.grades)
        db.query("UPDATE attempts SET score=?,provisional=? WHERE id=?",
                 {std::to_string(entry.second.first),std::to_string(entry.second.second),entry.first});
    for (const auto &entry : data.observations)
        db.query("UPDATE observations SET correct=?,eligible=? WHERE attempt_id=? AND skill_id=?",
                 {std::to_string(entry.second.correct),std::to_string(entry.second.eligible),entry.first,entry.second.skill});
    db.query("DELETE FROM student_skills"); db.query("DELETE FROM task_ratings");
    for (const auto &student : data.students)
        for (const auto &entry : student.second.skills) {
            const SkillState &s=entry.second;
            db.query("INSERT INTO student_skills VALUES(?,?,?,?,?)",
                     {student.first,entry.first,precise(s.mastery),precise(s.rating),std::to_string(s.observations)});
        }
    for (const auto &entry : data.difficulties)
        db.query("INSERT INTO task_ratings VALUES(?,?)", {entry.first,precise(entry.second)});
}
}
