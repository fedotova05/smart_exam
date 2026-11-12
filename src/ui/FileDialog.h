#ifndef SMARTEXAM_FILE_DIALOG_H
#define SMARTEXAM_FILE_DIALOG_H
#include <string>
namespace exam {
namespace ui {
std::string chooseLecture();
std::string choosePackage(bool save);
std::string chooseCsv();
} // namespace ui
} // namespace exam
#endif
