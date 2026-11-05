#ifndef SMARTEXAM_PACKAGE_SCREEN_H
#define SMARTEXAM_PACKAGE_SCREEN_H
#include "ui/Screen.h"
#include "ui/AppContext.h"
#include "application/jobs/JobQueue.h"
namespace exam {
namespace ui {
class PackageScreen : public Screen {
    AppContext &context;
    PackagePreview package;
    std::map<std::string, std::string> mapping;
    std::vector<std::string> exportIds;
    std::string path;
    std::unique_ptr<JobQueue> jobs;
    std::shared_ptr<PackagePreview> pendingPreview;
    unsigned screenId, requestId;
    bool inspecting;
    void inspectAsync();
    void importAsync();
    void waiting();
    void exportView();

  public:
    PackageScreen(const sf::Font &font, AppContext &state)
        : Screen(font), context(state), screenId(1), requestId(0), inspecting(false) {}
    void refresh();
    void tick() override;
    void leave() override;
};
} // namespace ui
} // namespace exam
#endif
