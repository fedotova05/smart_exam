#include "application/Demo.h"
#include "ui/App.h"
#include "io/Files.h"
#include <windows.h>
#include <iostream>

int main(int argc, char **argv) {
    // До создания любого окна или графического контекста.
    HMODULE user = GetModuleHandleA("user32.dll");
    typedef BOOL(WINAPI * DpiContext)(HANDLE);
    auto dpi = reinterpret_cast<DpiContext>(GetProcAddress(user, "SetProcessDpiAwarenessContext"));
    if (!dpi || !dpi(reinterpret_cast<HANDLE>(-4))) {
        typedef BOOL(WINAPI * DpiLegacy)();
        auto legacy = reinterpret_cast<DpiLegacy>(GetProcAddress(user, "SetProcessDPIAware"));
        if (legacy)
            legacy();
    }
    SetConsoleOutputCP(CP_UTF8);
    if (argc > 1 && std::string(argv[1]) == "--demo")
        return exam::runDemo();
    std::string option = argc > 1 ? argv[1] : "";
    try {
        exam::makeDirectories("data");
        if (option == "--create-demo" && exam::fileExists("data/demo-preview.db"))
            throw std::runtime_error("data/demo-preview.db уже существует");
        exam::Store store(option == "--capture"       ? ":memory:"
                          : option == "--create-demo" ? "data/demo-preview.db"
                                                      : "data/smart_exam.db");
        exam::SchoolService school(store);
        school.addDemonstrationCourse();
        if (option == "--create-demo") {
            std::cout << "Demo database created: " << school.tasks().size() << " tasks\n";
            return 0;
        }
        exam::ui::App app(school);
        return app.run(option == "--smoke", option == "--capture", option == "--widgets");
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
