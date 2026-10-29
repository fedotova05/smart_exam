#ifndef SMARTEXAM_PACKAGE_SERVICE_H
#define SMARTEXAM_PACKAGE_SERVICE_H
#include "application/SchoolService.h"
#include "io/Json.h"
namespace exam {
struct PackageItem {
    std::string sourceId, error;
    TaskVersion task;
};
struct PackagePreview {
    std::string id, directory;
    std::vector<std::string> skills;
    std::vector<PackageItem> items;
};
struct ImportReport {
    std::string id, message;
    bool imported;
};
class PackageService {
    SchoolService &school;
    std::string dataDirectory;

  public:
    PackageService(SchoolService &services, const std::string &data = "data")
        : school(services), dataDirectory(data) {}
    PackagePreview inspect(const std::string &path) const;
    std::vector<ImportReport> importPackage(const PackagePreview &package,
                                            const std::map<std::string, std::string> &skillMapping);
    void exportPackage(const std::string &path, const std::vector<std::string> &taskIds) const;
};
} // namespace exam
#endif
