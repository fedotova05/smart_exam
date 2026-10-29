#ifndef SMARTEXAM_DATABASE_H
#define SMARTEXAM_DATABASE_H
#include <string>
#include <vector>
struct sqlite3;
namespace exam {
typedef std::vector<std::string> Row;
class Database {
    sqlite3 *handle;

  public:
    explicit Database(const std::string &path);
    ~Database();
    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;
    std::vector<Row> query(const std::string &sql, const Row &values = Row());
    void script(const std::string &sql);
    void migrate(const std::string &directory);
};
class Transaction {
    Database &db;
    bool complete;

  public:
    explicit Transaction(Database &database);
    ~Transaction();
    void commit();
};
} // namespace exam
#endif
