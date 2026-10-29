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
        std::cout << "All 10 test groups passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
