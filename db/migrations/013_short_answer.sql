ALTER TABLE part_policies ADD COLUMN correct_answer TEXT NOT NULL DEFAULT '';
PRAGMA user_version=13;
