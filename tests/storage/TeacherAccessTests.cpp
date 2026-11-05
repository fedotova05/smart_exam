#include "../Test.h"
#include "application/SchoolService.h"
#include "application/review/ReviewService.h"
#include <functional>
using namespace exam;
namespace {
bool denied(const std::function<void()> &action) {
    try {
        action();
    } catch (const std::runtime_error &) {
        return true;
    }
    return false;
}
} // namespace
void testTeacherAccess() {
    Store store(":memory:");
    SchoolService school(store);
    REQUIRE(!store.hasTeacherPassword());
    REQUIRE(denied([&] { school.saveTopic("a", "Алгебра"); }));
    REQUIRE(denied([&] { ReviewService(store).replay(); }));
    REQUIRE(denied([&] { store.setTeacherPassword("short"); }));
    store.setTeacherPassword("teacher-password");
    REQUIRE(store.hasTeacherPassword());
    REQUIRE(!store.loginTeacher("wrong-password"));
    REQUIRE(store.loginTeacher("teacher-password"));
    school.saveTopic("a", "Алгебра");
    auto access = store.teacherSession();
    Store worker(":memory:");
    worker.useTeacherAccess(access);
    worker.requireTeacher();
    store.logoutTeacher();
    REQUIRE(denied([&] { worker.requireTeacher(); }));
    REQUIRE(denied([&] { school.saveTopic("b", "Дроби"); }));
    REQUIRE(denied([&] { store.setTeacherPassword("another-password"); }));
    REQUIRE(store.loginTeacher("teacher-password"));
    std::string student = school.createProfile("Мария");
    school.archiveProfile(student, true);
    REQUIRE(school.profiles().empty());
    REQUIRE(school.archivedProfiles().size() == 1);
    school.archiveProfile(student, false);
    REQUIRE(school.profiles().size() == 1);
    store.setTeacherPassword("changed-password");
    REQUIRE(!store.loginTeacher("teacher-password"));
    REQUIRE(store.loginTeacher("changed-password"));
}
