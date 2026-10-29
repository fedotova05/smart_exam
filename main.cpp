#include "application/Demo.h"
#include "ui/App.h"
#include "io/Files.h"
#include <windows.h>
#include <iostream>

int main(int argc, char **argv) {
    SetConsoleOutputCP(CP_UTF8);
    if (argc > 1 && std::string(argv[1]) == "--demo")
        return exam::runDemo();
    std::string option = argc > 1 ? argv[1] : "";
    try {
        exam::makeDirectories("data");
        exam::Store store(option == "--capture" ? ":memory:" : "data/smart_exam.db");
        exam::SchoolService school(store);
        school.addStarterContent();
        exam::ui::App app(school);
        return app.run(option == "--smoke", option == "--capture", option == "--widgets");
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
