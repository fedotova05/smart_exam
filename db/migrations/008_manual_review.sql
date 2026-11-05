CREATE TABLE review_verdicts(
 sequence INTEGER PRIMARY KEY AUTOINCREMENT, request_id TEXT UNIQUE NOT NULL,
 attempt_id TEXT REFERENCES attempts(id), part_id TEXT NOT NULL, status INTEGER NOT NULL,
 reviewer TEXT NOT NULL, reason TEXT NOT NULL, created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE TABLE attempt_models(
 attempt_id TEXT PRIMARY KEY REFERENCES attempts(id), version TEXT NOT NULL,
 initial REAL NOT NULL, learn REAL NOT NULL, slip REAL NOT NULL, guess REAL NOT NULL, k REAL NOT NULL
);
INSERT INTO attempt_models SELECT a.id,p.version,p.initial,p.learn,p.slip,p.guess,p.k
 FROM attempts a CROSS JOIN model_parameters p WHERE p.version='bkt-elo-1';
CREATE TABLE learning_events(sequence INTEGER PRIMARY KEY AUTOINCREMENT,
 attempt_id TEXT UNIQUE NOT NULL REFERENCES attempts(id), has_model INTEGER NOT NULL);
-- До этой миграции порядок завершений отдельно не сохранялся. Для старой базы фиксируем порядок попыток.
INSERT INTO learning_events(attempt_id,has_model) SELECT a.id,
 EXISTS(SELECT 1 FROM observations o WHERE o.attempt_id=a.id) FROM attempts a WHERE a.finished=1 ORDER BY a.rowid;
CREATE TABLE task_ratings(task_id TEXT PRIMARY KEY REFERENCES tasks(id), rating REAL NOT NULL);
CREATE TRIGGER frozen_attempt_model BEFORE UPDATE ON attempt_models
 BEGIN SELECT RAISE(ABORT,'Attempt model is immutable'); END;
CREATE TRIGGER immutable_review_update BEFORE UPDATE ON review_verdicts
 BEGIN SELECT RAISE(ABORT,'Review history is immutable'); END;
CREATE TRIGGER immutable_review_delete BEFORE DELETE ON review_verdicts
 BEGIN SELECT RAISE(ABORT,'Review history is immutable'); END;
PRAGMA user_version=8;
