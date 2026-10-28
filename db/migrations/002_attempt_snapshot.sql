CREATE TABLE attempt_skills(attempt_id TEXT REFERENCES attempts(id), skill_id TEXT REFERENCES skills(id), known INTEGER NOT NULL, mastery REAL NOT NULL, rating REAL NOT NULL, observations INTEGER NOT NULL, model_version TEXT NOT NULL, PRIMARY KEY(attempt_id,skill_id));
CREATE INDEX response_attempt ON responses(attempt_id,sequence);
CREATE INDEX help_attempt ON help_events(attempt_id,part_id);
CREATE INDEX attempts_session ON attempts(session_id,position);
PRAGMA user_version=2;
