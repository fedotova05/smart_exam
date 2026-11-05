ALTER TABLE sessions ADD COLUMN help_allowed INTEGER NOT NULL DEFAULT 1 CHECK(help_allowed IN (0,1));
ALTER TABLE sessions ADD COLUMN diagnosis_allowed INTEGER NOT NULL DEFAULT 1 CHECK(diagnosis_allowed IN (0,1));
ALTER TABLE lesson_templates ADD COLUMN help_allowed INTEGER NOT NULL DEFAULT 1 CHECK(help_allowed IN (0,1));
ALTER TABLE lesson_templates ADD COLUMN diagnosis_allowed INTEGER NOT NULL DEFAULT 1 CHECK(diagnosis_allowed IN (0,1));
PRAGMA user_version=11;
