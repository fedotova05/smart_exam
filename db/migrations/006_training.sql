ALTER TABLE sessions ADD COLUMN training INTEGER NOT NULL DEFAULT 0;
ALTER TABLE parts ADD COLUMN display_expression TEXT NOT NULL DEFAULT '';
CREATE TABLE session_skills (
    session_id TEXT NOT NULL REFERENCES sessions(id),
    skill_id TEXT NOT NULL REFERENCES skills(id),
    PRIMARY KEY(session_id, skill_id)
);
CREATE TABLE support_routes (
    session_id TEXT PRIMARY KEY REFERENCES sessions(id),
    origin_attempt TEXT NOT NULL REFERENCES attempts(id),
    active INTEGER NOT NULL DEFAULT 1
);
CREATE TABLE training_steps (
    attempt_id TEXT PRIMARY KEY REFERENCES attempts(id),
    origin_attempt TEXT NOT NULL REFERENCES attempts(id),
    stage TEXT NOT NULL,
    skill_id TEXT NOT NULL REFERENCES skills(id)
);
PRAGMA user_version=6;
