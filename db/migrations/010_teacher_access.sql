CREATE TABLE teacher_credentials (
    id INTEGER PRIMARY KEY CHECK(id=1),
    salt TEXT NOT NULL,
    password_hash TEXT NOT NULL,
    iterations INTEGER NOT NULL
);
CREATE TABLE archived_profiles (
    profile_id TEXT PRIMARY KEY REFERENCES profiles(id)
);
PRAGMA user_version=10;
