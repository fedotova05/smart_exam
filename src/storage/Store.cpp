#include "Store.h"
#include "core/GraphCodec.h"
#include <stdexcept>
#include <set>
namespace exam {
namespace {
std::string number(int value) { return std::to_string(value); }
void require(bool condition, const std::string& message) { if (!condition) throw std::runtime_error(message); }
AttemptRecord readAttempt(const Row& row) {
    AttemptRecord a;
    a.id=row[0]; a.sessionId=row[1]; a.position=std::stoi(row[2]); a.taskId=row[3];
    a.finished=row[4]=="1"; a.score=std::stoi(row[5]); a.provisional=row[6]=="1";
    return a;
}
ResponseRecord readResponse(const Row& row) {
    ResponseRecord r;
    r.requestId=row[0]; r.attemptId=row[1]; r.partId=row[2]; r.answer.text=row[3];
    r.graph=row[4]; r.result=CheckResult(static_cast<CheckStatus>(std::stoi(row[5])),row[6]);
    r.finalAnswer=row[7]=="1"; r.operation=row[8]; r.operand=row[9];
    r.answer.graph=decodeGraph(r.graph);
    return r;
}
const std::string responseColumns="request_id,attempt_id,part_id,text,graph,status,message,is_final,operation,operand";
}
Store::Store(const std::string& path, const std::string& migrations) : db(path) { db.migrate(migrations); }
void Store::saveProfile(const std::string& id, const std::string& name) {
    require(!id.empty() && !name.empty(), "Empty profile");
    db.query("INSERT INTO profiles VALUES(?,?) ON CONFLICT(id) DO UPDATE SET name=excluded.name", {id,name});
}
void Store::saveTopic(const std::string& id, const std::string& name) {
    db.query("INSERT INTO topics VALUES(?,?) ON CONFLICT(id) DO UPDATE SET name=excluded.name", {id,name});
}
void Store::saveSkill(const std::string& id, const std::string& topic, const std::string& name) {
    db.query("INSERT INTO skills VALUES(?,?,?) ON CONFLICT(id) DO UPDATE SET name=excluded.name,topic_id=excluded.topic_id", {id,topic,name});
}
void Store::savePrerequisite(const std::string& skill, const std::string& prerequisite) {
    db.query("INSERT OR IGNORE INTO prerequisites VALUES(?,?)", {skill,prerequisite});
}
std::map<std::string,std::vector<std::string> > Store::skillGraph() {
    std::map<std::string,std::vector<std::string> > result;
    for (const Row& row : db.query("SELECT id FROM skills ORDER BY id")) result[row[0]];
    for (const Row& row : db.query("SELECT skill_id,prerequisite_id FROM prerequisites ORDER BY skill_id,prerequisite_id")) result[row[0]].push_back(row[1]);
    return result;
}
void Store::saveDraft(const TaskVersion& value) {
    require(!value.id.empty() && !value.title.empty(), "Empty task identifier or title");
    Transaction transaction(db);
    std::vector<Row> old=db.query("SELECT published FROM tasks WHERE id=?",{value.id});
    require(old.empty() || old[0][0]=="0", "Published version is immutable; create a new identifier");
    db.query("DELETE FROM part_skills WHERE task_id=?",{value.id});
    db.query("DELETE FROM parts WHERE task_id=?",{value.id});
    db.query("INSERT INTO tasks VALUES(?,?,0,0,?,?,?,?) ON CONFLICT(id) DO UPDATE SET title=excluded.title,generator=excluded.generator,seed=excluded.seed,parameters=excluded.parameters,difficulty=excluded.difficulty", {value.id,value.title,value.generator,std::to_string(value.seed),value.parameters,std::to_string(value.difficulty)});
    for (const TaskPart& part : value.parts) {
        require(!part.id.empty() && part.maxScore>0, "Invalid task part");
        db.query("INSERT INTO parts VALUES(?,?,?,?,?,?)",{value.id,part.id,number(static_cast<int>(part.kind)),part.prompt,part.expression,number(part.maxScore)});
        for (const std::string& skill : part.skills)
            db.query("INSERT INTO part_skills VALUES(?,?,?)",{value.id,part.id,skill});
    }
    transaction.commit();
}
void Store::publish(const std::string& id) {
    Transaction transaction(db);
    TaskVersion value=task(id);
    require(!value.parts.empty() && !value.archived, "Cannot publish empty or archived task");
    for (const TaskPart& part : value.parts) require(!part.expression.empty() && !part.prompt.empty() && !part.skills.empty(), "Incomplete task part");
    db.query("UPDATE tasks SET published=1 WHERE id=?",{id});
    transaction.commit();
}
void Store::archive(const std::string& id) { task(id); db.query("UPDATE tasks SET archived=1 WHERE id=?",{id}); }
TaskVersion Store::task(const std::string& id) {
    std::vector<Row> rows=db.query("SELECT id,title,published,archived,generator,seed,parameters,difficulty FROM tasks WHERE id=?",{id});
    require(!rows.empty(), "Task not found");
    const Row& r=rows[0]; TaskVersion t;
    t.id=r[0]; t.title=r[1]; t.published=r[2]=="1"; t.archived=r[3]=="1";
    t.generator=r[4]; t.seed=static_cast<unsigned>(std::stoul(r[5])); t.parameters=r[6]; t.difficulty=std::stod(r[7]);
    for (const Row& p : db.query("SELECT id,kind,prompt,expression,max_score FROM parts WHERE task_id=? ORDER BY id",{id})) {
        TaskPart part; part.id=p[0]; part.kind=static_cast<PartKind>(std::stoi(p[1]));
        part.prompt=p[2]; part.expression=p[3]; part.maxScore=std::stoi(p[4]);
        for (const Row& s : db.query("SELECT skill_id FROM part_skills WHERE task_id=? AND part_id=? ORDER BY skill_id",{id,part.id})) part.skills.push_back(s[0]);
        t.parts.push_back(part);
    }
    return t;
}
std::vector<TaskVersion> Store::publishedTasks() {
    std::vector<TaskVersion> result;
    for (const Row& r : db.query("SELECT id FROM tasks WHERE published=1 AND archived=0 ORDER BY id")) result.push_back(task(r[0]));
    return result;
}
void Store::createSession(const SessionRecord& value) {
    require(!value.id.empty() && value.settings.taskLimit>0 && value.settings.taskLimit<=1000, "Invalid session");
    require(value.settings.mode!=SessionMode::Fixed || (!value.tasks.empty() && value.tasks.size()<=static_cast<std::size_t>(value.settings.taskLimit)), "Invalid fixed set");
    Transaction transaction(db);
    db.query("INSERT INTO sessions VALUES(?,?,?,?,?,?)",{value.id,value.studentId,number(static_cast<int>(value.settings.mode)),number(static_cast<int>(value.settings.feedback)),number(value.settings.guided),number(value.settings.taskLimit)});
    for (std::size_t i=0;i<value.tasks.size();++i) {
        TaskVersion t=task(value.tasks[i]); require(t.published && !t.archived, "Task unavailable");
        db.query("INSERT INTO session_tasks VALUES(?,?,?)",{value.id,number(static_cast<int>(i)),t.id});
    }
    transaction.commit();
}
SessionRecord Store::session(const std::string& id) {
    std::vector<Row> rows=db.query("SELECT student_id,mode,feedback,guided,task_limit FROM sessions WHERE id=?",{id});
    require(!rows.empty(), "Session not found"); const Row& r=rows[0];
    SessionRecord s; s.id=id; s.studentId=r[0]; s.settings.mode=static_cast<SessionMode>(std::stoi(r[1]));
    s.settings.feedback=static_cast<FeedbackPolicy>(std::stoi(r[2])); s.settings.guided=r[3]=="1"; s.settings.taskLimit=std::stoi(r[4]);
    for (const Row& t : db.query("SELECT task_id FROM session_tasks WHERE session_id=? ORDER BY position",{id})) s.tasks.push_back(t[0]);
    return s;
}
AttemptRecord Store::openAttempt(const std::string& sessionId, int position, const std::string& taskId) {
    Transaction transaction(db);
    SessionRecord s=session(sessionId);
    std::vector<AttemptRecord> old=attempts(sessionId);
    if (position<static_cast<int>(old.size())) {
        require(old[position].taskId==taskId, "Attempt task cannot change");
        transaction.commit(); return old[position];
    }
    require(position==static_cast<int>(old.size()) && position<s.settings.taskLimit, "Invalid attempt position");
    require(old.empty() || old.back().finished, "Finish current attempt first");
    require(db.query("SELECT 1 FROM attempts a JOIN sessions s ON s.id=a.session_id WHERE s.student_id=? AND a.finished=0",{s.studentId}).empty(), "У ученика уже есть незавершённая попытка");
    TaskVersion t=task(taskId); require(t.published, "Draft task cannot be assigned");
    if (s.settings.mode==SessionMode::Fixed) require(position<static_cast<int>(s.tasks.size()) && s.tasks[position]==taskId, "Fixed set cannot change");
    else require(!t.archived, "Task archived");
    std::string id=sessionId+":"+number(position);
    db.query("INSERT INTO attempts(id,session_id,position,task_id) VALUES(?,?,?,?)",{id,sessionId,number(position),taskId});
    db.query("INSERT INTO attempt_skills SELECT ?,k.id,CASE WHEN s.skill_id IS NULL THEN 0 ELSE 1 END,COALESCE(s.mastery,0.2),COALESCE(s.rating,1000),COALESCE(s.observations,0),'bkt-elo-1' FROM skills k LEFT JOIN student_skills s ON s.skill_id=k.id AND s.student_id=?",{id,s.studentId});
    transaction.commit(); return attempt(id);
}
AttemptRecord Store::attempt(const std::string& id) {
    std::vector<Row> rows=db.query("SELECT id,session_id,position,task_id,finished,score,provisional FROM attempts WHERE id=?",{id});
    require(!rows.empty(), "Attempt not found"); return readAttempt(rows[0]);
}
std::vector<AttemptRecord> Store::attempts(const std::string& id) {
    std::vector<AttemptRecord> result;
    for (const Row& r:db.query("SELECT id,session_id,position,task_id,finished,score,provisional FROM attempts WHERE session_id=? ORDER BY position",{id})) result.push_back(readAttempt(r));
    return result;
}
bool Store::response(const std::string& id, ResponseRecord& output) {
    std::vector<Row> rows=db.query("SELECT "+responseColumns+" FROM responses WHERE request_id=?",{id});
    if (rows.empty()) return false;
    output=readResponse(rows[0]); return true;
}
void Store::saveResponse(const ResponseRecord& r) {
    require(!r.requestId.empty(), "Empty request identifier");
    Transaction transaction(db);
    ResponseRecord old;
    if (response(r.requestId,old)) {
        require(old.attemptId==r.attemptId && old.partId==r.partId && old.answer.text==r.answer.text && old.graph==r.graph && old.finalAnswer==r.finalAnswer && old.operation==r.operation && old.operand==r.operand, "Request identifier reused with different content");
        transaction.commit(); return;
    }
    AttemptRecord a=attempt(r.attemptId); require(!a.finished, "Attempt is finished");
    bool found=false; for (const TaskPart& p:task(a.taskId).parts) if (p.id==r.partId) found=true;
    require(found, "Part not found");
    db.query("INSERT INTO responses("+responseColumns+") VALUES(?,?,?,?,?,?,?,?,?,?)",{r.requestId,r.attemptId,r.partId,r.answer.text,r.graph,number(static_cast<int>(r.result.status)),r.result.message,number(r.finalAnswer),r.operation,r.operand});
    transaction.commit();
}
std::vector<ResponseRecord> Store::responses(const std::string& id) {
    std::vector<ResponseRecord> result;
    for (const Row& r:db.query("SELECT "+responseColumns+" FROM responses WHERE attempt_id=? ORDER BY sequence",{id})) result.push_back(readResponse(r));
    return result;
}
void Store::addHelp(const std::string& id,const std::string& attemptId,const std::string& partId,const std::string& kind) {
    Transaction transaction(db);
    std::vector<Row> old=db.query("SELECT attempt_id,part_id,kind FROM help_events WHERE request_id=?",{id});
    if (!old.empty()) { require(old[0]==Row({attemptId,partId,kind}), "Help identifier reused"); transaction.commit(); return; }
    require(!id.empty() && !kind.empty(), "Empty help event");
    AttemptRecord a=attempt(attemptId); require(!a.finished, "Attempt is finished");
    bool found=false; for (const TaskPart& p:task(a.taskId).parts) if(p.id==partId) found=true;
    require(found, "Part not found");
    db.query("INSERT INTO help_events(request_id,attempt_id,part_id,kind) VALUES(?,?,?,?)",{id,attemptId,partId,kind});
    transaction.commit();
}
bool Store::usedHelp(const std::string& id,const std::string& part) { return !db.query("SELECT 1 FROM help_events WHERE attempt_id=? AND part_id=?",{id,part}).empty(); }
void Store::finish(const std::string& id,int score,bool provisional,const std::vector<Observation>& observations) {
    Transaction transaction(db);
    AttemptRecord a=attempt(id);
    if (a.finished) { transaction.commit(); return; }
    std::string student=session(a.sessionId).studentId;
    for (const Observation& o:observations) {
        db.query("INSERT INTO observations VALUES(?,?,?,?,?,?,?,?,?)",{id,o.skill,number(o.correct),number(o.eligible),std::to_string(o.before.mastery),std::to_string(o.before.rating),number(o.before.known),number(o.before.observations),"bkt-elo-1"});
        if (o.eligible) db.query("INSERT INTO student_skills VALUES(?,?,?,?,?) ON CONFLICT(student_id,skill_id) DO UPDATE SET mastery=excluded.mastery,rating=excluded.rating,observations=excluded.observations",{student,o.skill,std::to_string(o.after.mastery),std::to_string(o.after.rating),number(o.after.observations)});
    }
    db.query("UPDATE attempts SET finished=1,score=?,provisional=? WHERE id=?",{number(score),number(provisional),id});
    transaction.commit();
}
StudentSnapshot Store::snapshot(const std::string& student) {
    StudentSnapshot result; result.studentId=student;
    for (const Row& r:db.query("SELECT skill_id,mastery,rating,observations FROM student_skills WHERE student_id=?",{student})) {
        SkillState s; s.known=true; s.mastery=std::stod(r[1]); s.rating=std::stod(r[2]); s.observations=std::stoi(r[3]); result.skills[r[0]]=s;
    }
    return result;
}
int Store::observationCount(const std::string& id) { return std::stoi(db.query("SELECT count(*) FROM observations WHERE attempt_id=?",{id})[0][0]); }
StudentSnapshot Store::attemptSnapshot(const std::string& id) {
    AttemptRecord a=attempt(id);
    StudentSnapshot result; result.studentId=session(a.sessionId).studentId;
    for(const Row& r:db.query("SELECT skill_id,known,mastery,rating,observations,model_version FROM attempt_skills WHERE attempt_id=?",{id})) {
        SkillState state; state.known=r[1]=="1"; state.mastery=std::stod(r[2]); state.rating=std::stod(r[3]); state.observations=std::stoi(r[4]);
        result.skills[r[0]]=state; result.modelVersion=r[5];
    }
    return result;
}
}
