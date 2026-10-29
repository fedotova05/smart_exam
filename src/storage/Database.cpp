#include "Database.h"
#include "sqlite3.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
namespace exam {
Database::Database(const std::string &path) : handle(0) {
    if (sqlite3_open(path.c_str(), &handle) != SQLITE_OK) {
        std::string error = sqlite3_errmsg(handle);
        sqlite3_close(handle);
        throw std::runtime_error(error);
    }
    sqlite3_busy_timeout(handle, 3000);
    script("PRAGMA foreign_keys=ON;");
}
Database::~Database() {
    sqlite3_close(handle);
}
std::vector<Row> Database::query(const std::string &sql, const Row &values) {
    sqlite3_stmt *statement = 0;
    if (sqlite3_prepare_v2(handle, sql.c_str(), -1, &statement, 0) != SQLITE_OK)
        throw std::runtime_error(sqlite3_errmsg(handle));
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (sqlite3_bind_text(statement, static_cast<int>(i + 1), values[i].c_str(), -1,
                              SQLITE_TRANSIENT) != SQLITE_OK) {
            sqlite3_finalize(statement);
            throw std::runtime_error("Cannot bind SQL parameter");
        }
    }
    std::vector<Row> rows;
    int code;
    while ((code = sqlite3_step(statement)) == SQLITE_ROW) {
        Row row;
        for (int i = 0; i < sqlite3_column_count(statement); ++i) {
            const unsigned char *text = sqlite3_column_text(statement, i);
            row.push_back(text ? reinterpret_cast<const char *>(text) : "");
        }
        rows.push_back(row);
    }
    std::string error = sqlite3_errmsg(handle);
    sqlite3_finalize(statement);
    if (code != SQLITE_DONE)
        throw std::runtime_error(error);
    return rows;
}
void Database::script(const std::string &sql) {
    char *message = 0;
    if (sqlite3_exec(handle, sql.c_str(), 0, 0, &message) != SQLITE_OK) {
        std::string error = message ? message : "SQL error";
        sqlite3_free(message);
        throw std::runtime_error(error);
    }
}
void Database::migrate(const std::string &directory) {
    int version = std::stoi(query("PRAGMA user_version")[0][0]);
    if (version > 5)
        throw std::runtime_error("Database version is newer than application");
    const char *files[] = {"001_initial.sql", "002_attempt_snapshot.sql",
                           "003_session_completion.sql", "004_interface_data.sql",
                           "005_graph_settings.sql"};
    for (int next = version; next < 5; ++next) {
        std::ifstream input((directory + "/" + files[next]).c_str());
        if (!input)
            throw std::runtime_error("Migration file not found");
        std::ostringstream text;
        text << input.rdbuf();
        Transaction transaction(*this);
        script(text.str());
        transaction.commit();
    }
}
Transaction::Transaction(Database &database) : db(database), complete(false) {
    db.script("BEGIN IMMEDIATE");
}
Transaction::~Transaction() {
    if (!complete) {
        try {
            db.script("ROLLBACK");
        } catch (...) {
        }
    }
}
void Transaction::commit() {
    db.script("COMMIT");
    complete = true;
}
} // namespace exam
