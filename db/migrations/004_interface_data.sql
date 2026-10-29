ALTER TABLE tasks ADD COLUMN description TEXT NOT NULL DEFAULT '';
ALTER TABLE tasks ADD COLUMN common_function TEXT NOT NULL DEFAULT '';
CREATE TABLE task_resources(task_id TEXT REFERENCES tasks(id), path TEXT NOT NULL, PRIMARY KEY(task_id,path));
CREATE TABLE skill_positions(skill_id TEXT PRIMARY KEY REFERENCES skills(id), x REAL NOT NULL, y REAL NOT NULL);
CREATE TRIGGER immutable_task_text BEFORE UPDATE ON tasks WHEN OLD.published=1 AND (NEW.description<>OLD.description OR NEW.common_function<>OLD.common_function) BEGIN SELECT RAISE(ABORT,'Published version is immutable'); END;
CREATE TRIGGER immutable_resource_insert BEFORE INSERT ON task_resources WHEN (SELECT published FROM tasks WHERE id=NEW.task_id)=1 BEGIN SELECT RAISE(ABORT,'Published version is immutable'); END;
CREATE TRIGGER immutable_resource_update BEFORE UPDATE ON task_resources WHEN (SELECT published FROM tasks WHERE id=OLD.task_id)=1 OR (SELECT published FROM tasks WHERE id=NEW.task_id)=1 BEGIN SELECT RAISE(ABORT,'Published version is immutable'); END;
CREATE TRIGGER immutable_resource_delete BEFORE DELETE ON task_resources WHEN (SELECT published FROM tasks WHERE id=OLD.task_id)=1 BEGIN SELECT RAISE(ABORT,'Published version is immutable'); END;
PRAGMA user_version=4;
