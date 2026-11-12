CREATE TABLE lectures(
 id TEXT PRIMARY KEY, title TEXT NOT NULL, body TEXT NOT NULL,
 attachment TEXT NOT NULL DEFAULT '', attachment_name TEXT NOT NULL DEFAULT '',
 published INTEGER NOT NULL DEFAULT 0 CHECK(published IN (0,1)),
 archived INTEGER NOT NULL DEFAULT 0 CHECK(archived IN (0,1)),
 revision INTEGER NOT NULL DEFAULT 1, updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE TABLE lecture_skills(lecture_id TEXT NOT NULL REFERENCES lectures(id),
 skill_id TEXT NOT NULL REFERENCES skills(id), PRIMARY KEY(lecture_id,skill_id));
CREATE INDEX lecture_skill_lookup ON lecture_skills(skill_id,lecture_id);
CREATE TABLE exam_tasks(task_id TEXT PRIMARY KEY REFERENCES tasks(id),
 exam TEXT NOT NULL, subject TEXT NOT NULL, year INTEGER NOT NULL,
 number INTEGER NOT NULL CHECK(number BETWEEN 1 AND 19), category INTEGER NOT NULL,
 source TEXT NOT NULL, external_id TEXT NOT NULL, source_url TEXT NOT NULL,
 fingerprint TEXT NOT NULL, fetched_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
 UNIQUE(exam,subject,year,source,external_id,fingerprint));
CREATE INDEX exam_catalog ON exam_tasks(exam,subject,year,number,category);
PRAGMA user_version=14;
