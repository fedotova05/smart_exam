CREATE TABLE part_policies(task_id TEXT, part_id TEXT, require_steps INTEGER NOT NULL,
 decimal_places INTEGER NOT NULL, tolerance REAL NOT NULL, PRIMARY KEY(task_id,part_id),
 FOREIGN KEY(task_id,part_id) REFERENCES parts(task_id,id) ON DELETE CASCADE);
CREATE TABLE part_choices(task_id TEXT, part_id TEXT, position INTEGER NOT NULL,
 text TEXT NOT NULL, correct INTEGER NOT NULL, PRIMARY KEY(task_id,part_id,position),
 FOREIGN KEY(task_id,part_id) REFERENCES parts(task_id,id) ON DELETE CASCADE);
CREATE TRIGGER immutable_policy_update BEFORE UPDATE ON part_policies
 WHEN (SELECT published FROM tasks WHERE id=OLD.task_id)=1 OR (SELECT published FROM tasks WHERE id=NEW.task_id)=1
 BEGIN SELECT RAISE(ABORT,'Published policy is immutable'); END;
CREATE TRIGGER immutable_policy_insert BEFORE INSERT ON part_policies WHEN (SELECT published FROM tasks WHERE id=NEW.task_id)=1
 BEGIN SELECT RAISE(ABORT,'Published policy is immutable'); END;
CREATE TRIGGER immutable_policy_delete BEFORE DELETE ON part_policies WHEN (SELECT published FROM tasks WHERE id=OLD.task_id)=1
 BEGIN SELECT RAISE(ABORT,'Published policy is immutable'); END;
CREATE TRIGGER immutable_choice_update BEFORE UPDATE ON part_choices
 WHEN (SELECT published FROM tasks WHERE id=OLD.task_id)=1 OR (SELECT published FROM tasks WHERE id=NEW.task_id)=1
 BEGIN SELECT RAISE(ABORT,'Published choice is immutable'); END;
CREATE TRIGGER immutable_choice_insert BEFORE INSERT ON part_choices WHEN (SELECT published FROM tasks WHERE id=NEW.task_id)=1
 BEGIN SELECT RAISE(ABORT,'Published choice is immutable'); END;
CREATE TRIGGER immutable_choice_delete BEFORE DELETE ON part_choices WHEN (SELECT published FROM tasks WHERE id=OLD.task_id)=1
 BEGIN SELECT RAISE(ABORT,'Published choice is immutable'); END;
PRAGMA user_version=9;
