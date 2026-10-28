#include "Test.h"
#include "core/Services.h"

void testCore() {
    exam::CheckResult result;
    REQUIRE(result.status == exam::CheckStatus::Unsupported);
    exam::UnavailableGraphChecker checker;
    REQUIRE(checker.check(exam::TaskPart(), exam::GraphAnswer()).status == exam::CheckStatus::Unsupported);
}
int main() {
    try {
        testCore();
        testStorage();
        testMath();
        testChecker();
        testGeneration();
        std::cout << "All tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
