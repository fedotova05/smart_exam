#ifndef SMARTEXAM_PACKAGE_SCREEN_H
#define SMARTEXAM_PACKAGE_SCREEN_H
#include "ui/Screen.h"
#include "ui/AppContext.h"
namespace exam {
namespace ui {
class PackageScreen : public Screen {
    AppContext &context;
    PackagePreview package;
    std::map<std::string, std::string> mapping;
    std::vector<std::string> exportIds;
    std::string path;
    void exportView();

  public:
    PackageScreen(const sf::Font &font, AppContext &state) : Screen(font), context(state) {}
    void refresh();
};
} // namespace ui
} // namespace exam
#endif
