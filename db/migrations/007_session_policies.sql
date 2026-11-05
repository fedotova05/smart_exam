ALTER TABLE sessions ADD COLUMN resume_policy INTEGER NOT NULL DEFAULT 0;
ALTER TABLE sessions ADD COLUMN time_limit INTEGER NOT NULL DEFAULT 0;
CREATE TABLE session_clocks(
 session_id TEXT PRIMARY KEY REFERENCES sessions(id),
 started INTEGER NOT NULL, last_seen INTEGER NOT NULL, elapsed INTEGER NOT NULL DEFAULT 0,
 paused INTEGER NOT NULL DEFAULT 0, owner TEXT NOT NULL, stopped TEXT NOT NULL DEFAULT ''
);
CREATE TABLE lesson_templates(
 id TEXT PRIMARY KEY, name TEXT NOT NULL, mode INTEGER NOT NULL, feedback INTEGER NOT NULL,
 guided INTEGER NOT NULL, training INTEGER NOT NULL, task_limit INTEGER NOT NULL,
 resume_policy INTEGER NOT NULL, time_limit INTEGER NOT NULL
);
CREATE TABLE template_tasks(template_id TEXT REFERENCES lesson_templates(id), position INTEGER,
 task_id TEXT REFERENCES tasks(id), PRIMARY KEY(template_id,position));
CREATE TABLE template_skills(template_id TEXT REFERENCES lesson_templates(id), skill_id TEXT REFERENCES skills(id),
 PRIMARY KEY(template_id,skill_id));
CREATE TABLE answer_drafts(attempt_id TEXT REFERENCES attempts(id), part_id TEXT NOT NULL,
 text TEXT NOT NULL, graph TEXT NOT NULL, PRIMARY KEY(attempt_id,part_id));
PRAGMA user_version=7;
