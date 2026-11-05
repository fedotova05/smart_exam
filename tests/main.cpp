#include "Test.h"
#include "core/Services.h"

void testCore() {
    exam::CheckResult result;
    REQUIRE(result.status == exam::CheckStatus::Unsupported);
    exam::UnavailableGraphChecker checker;
    REQUIRE(checker.check(exam::TaskPart(), exam::GraphAnswer()).status ==
            exam::CheckStatus::Unsupported);
}
int main() {
    try {
        testCore();
        std::cout << "OK: core\n";
        testStorage();
        std::cout << "OK: storage\n";
        testMath();
        std::cout << "OK: rational and parser\n";
        testChecker();
        std::cout << "OK: equations and steps\n";
        testGeneration();
        std::cout << "OK: 250 generated tasks\n";
        testSession();
        std::cout << "OK: sessions\n";
        testAdaptation();
        std::cout << "OK: BKT, Elo and selection\n";
        testGeometry();
        std::cout << "OK: graph geometry and coordinates\n";
        testJsonPackages();
        std::cout << "OK: JSON and package roundtrip\n";
        testSchool();
        std::cout << "OK: editor, knowledge and graphical sessions\n";
        testTraining();
        std::cout << "OK: scoped training, diagnosis, return, resume and fixed assessment\n";
        testRecovery();
        std::cout << "OK: templates, drafts, restart and time policies\n";
        testReview();
        std::cout << "OK: review history, BKT and shared Elo replay\n";
        std::cout << "All test groups passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
