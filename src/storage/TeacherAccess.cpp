#include "Store.h"
#include <windows.h>
#include <stdexcept>

namespace exam {
namespace {
// Windows CNG: PBKDF2-HMAC-SHA256. Пароль не сохраняется в базе.
class PasswordAlgorithm {
    HMODULE library;
    void *algorithm;
    typedef LONG(WINAPI *Open)(void **, LPCWSTR, LPCWSTR, ULONG);
    typedef LONG(WINAPI *Close)(void *, ULONG);
    typedef LONG(WINAPI *Random)(void *, unsigned char *, ULONG, ULONG);
    typedef LONG(WINAPI *Derive)(void *, unsigned char *, ULONG, unsigned char *, ULONG, ULONGLONG,
                                 unsigned char *, ULONG, ULONG);
    Close close;
    Random random;
    Derive derive;

  public:
    PasswordAlgorithm() : library(0), algorithm(0), close(0), random(0), derive(0) {
        wchar_t path[MAX_PATH];
        UINT length = GetSystemDirectoryW(path, MAX_PATH);
        if (!length || length >= MAX_PATH - 12)
            throw std::runtime_error("Не найден каталог Windows");
        std::wstring full(path);
        full += L"\\bcrypt.dll";
        library = LoadLibraryW(full.c_str());
        if (library) {
            Open open =
                reinterpret_cast<Open>(GetProcAddress(library, "BCryptOpenAlgorithmProvider"));
            close =
                reinterpret_cast<Close>(GetProcAddress(library, "BCryptCloseAlgorithmProvider"));
            random = reinterpret_cast<Random>(GetProcAddress(library, "BCryptGenRandom"));
            derive = reinterpret_cast<Derive>(GetProcAddress(library, "BCryptDeriveKeyPBKDF2"));
            if (open && close && random && derive && open(&algorithm, L"SHA256", 0, 8) >= 0)
                return;
            FreeLibrary(library);
        }
        throw std::runtime_error("Windows CNG недоступен");
    }
    ~PasswordAlgorithm() {
        close(algorithm, 0);
        FreeLibrary(library);
    }
    std::vector<unsigned char> salt() {
        std::vector<unsigned char> value(16);
        if (random(0, value.data(), 16, 2) < 0)
            throw std::runtime_error("Не удалось получить случайную соль");
        return value;
    }
    std::vector<unsigned char> hash(const std::string &password, std::vector<unsigned char> salt,
                                    unsigned iterations) {
        std::vector<unsigned char> result(32);
        if (derive(algorithm,
                   reinterpret_cast<unsigned char *>(const_cast<char *>(password.data())),
                   static_cast<ULONG>(password.size()), salt.data(),
                   static_cast<ULONG>(salt.size()), iterations, result.data(), 32, 0) < 0)
            throw std::runtime_error("Не удалось проверить пароль");
        return result;
    }
};
std::string hex(const std::vector<unsigned char> &bytes) {
    const char *digits = "0123456789abcdef";
    std::string result;
    for (unsigned char value : bytes) {
        result += digits[value >> 4];
        result += digits[value & 15];
    }
    return result;
}
std::vector<unsigned char> unhex(const std::string &text) {
    std::vector<unsigned char> result;
    if (text.size() != 32)
        throw std::runtime_error("Повреждена запись пароля");
    for (std::size_t i = 0; i < text.size(); i += 2) {
        unsigned value = 0;
        for (unsigned j = 0; j < 2; ++j) {
            char c = text[i + j];
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
                throw std::runtime_error("Повреждена запись пароля");
            value = value * 16 + (c <= '9' ? c - '0' : c - 'a' + 10);
        }
        result.push_back(static_cast<unsigned char>(value));
    }
    return result;
}
} // namespace
bool Store::hasTeacherPassword() {
    return !db.query("SELECT 1 FROM teacher_credentials WHERE id=1").empty();
}
void Store::setTeacherPassword(const std::string &password) {
    unsigned characters = 0;
    for (unsigned char c : password)
        if ((c & 0xc0) != 0x80)
            ++characters;
    if (characters < 8 || password.size() > 128 || password.find('\0') != std::string::npos)
        throw std::runtime_error("Нужно не менее 8 символов. Максимум — 128 байтов UTF-8.");
    if (hasTeacherPassword())
        requireTeacher();
    PasswordAlgorithm algorithm;
    const auto salt = algorithm.salt();
    const std::string hash = hex(algorithm.hash(password, salt, 600000));
    Transaction transaction(db);
    // Повторная проверка защищает первичную настройку при двух открытых окнах.
    if (hasTeacherPassword())
        requireTeacher();
    db.query("INSERT INTO teacher_credentials VALUES(1,?,?,600000) ON CONFLICT(id) "
             "DO UPDATE SET salt=excluded.salt,password_hash=excluded.password_hash,"
             "iterations=excluded.iterations",
             {hex(salt), hash});
    transaction.commit();
    logoutTeacher();
}
bool Store::loginTeacher(const std::string &password) {
    logoutTeacher();
    if (password.size() > 128)
        return false;
    auto rows =
        db.query("SELECT salt,password_hash,iterations FROM teacher_credentials WHERE id=1");
    if (rows.empty())
        return false;
    unsigned iterations = static_cast<unsigned>(std::stoul(rows[0][2]));
    if (iterations < 100000 || iterations > 2000000 || rows[0][1].size() != 64)
        throw std::runtime_error("Повреждена запись пароля");
    PasswordAlgorithm algorithm;
    std::string actual = hex(algorithm.hash(password, unhex(rows[0][0]), iterations));
    unsigned difference = 0;
    for (std::size_t i = 0; i < actual.size(); ++i)
        difference |= static_cast<unsigned char>(actual[i] ^ rows[0][1][i]);
    if (difference)
        return false;
    teacherAccess.reset(new TeacherAccess(databasePath));
    return true;
}
void Store::logoutTeacher() {
    if (teacherAccess)
        teacherAccess->valid = false;
    teacherAccess.reset();
}
void Store::requireTeacher() const {
    if (!teacherAccess || !teacherAccess->valid || teacherAccess->database != databasePath)
        throw std::runtime_error("Войдите в кабинет учителя");
}
void Store::useTeacherAccess(const std::shared_ptr<TeacherAccess> &access) {
    teacherAccess = access;
    requireTeacher();
}
void Store::archiveProfile(const std::string &id, bool archived) {
    requireTeacher();
    if (archived) {
        for (const auto &lesson : studentSessions(id))
            if (!lesson.closed)
                throw std::runtime_error("Сначала завершите занятие ученика");
        db.query("INSERT OR IGNORE INTO archived_profiles VALUES(?)", {id});
    } else
        db.query("DELETE FROM archived_profiles WHERE profile_id=?", {id});
}
std::vector<std::pair<std::string, std::string>> Store::archivedProfiles() {
    requireTeacher();
    std::vector<std::pair<std::string, std::string>> result;
    for (const Row &row : db.query("SELECT id,name FROM profiles WHERE id IN "
                                   "(SELECT profile_id FROM archived_profiles) ORDER BY name,id"))
        result.push_back({row[0], row[1]});
    return result;
}
} // namespace exam
