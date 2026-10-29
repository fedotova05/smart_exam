#include "FileDialog.h"
#include <windows.h>
#include <commdlg.h>
#include <stdexcept>
namespace exam {
namespace ui {
std::string choosePackage(bool save) {
    wchar_t path[32768] = {0};
    OPENFILENAMEW dialog = {};
    dialog.lStructSize = sizeof(dialog);
    dialog.lpstrFilter = L"JSON (*.json)\0*.json\0";
    dialog.lpstrFile = path;
    dialog.nMaxFile = 32768;
    dialog.lpstrDefExt = L"json";
    dialog.Flags =
        OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    BOOL selected = save ? GetSaveFileNameW(&dialog) : GetOpenFileNameW(&dialog);
    if (!selected) {
        if (CommDlgExtendedError())
            throw std::runtime_error("Не удалось открыть выбор файла");
        return "";
    }
    int size = WideCharToMultiByte(CP_UTF8, 0, path, -1, 0, 0, 0, 0);
    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, path, -1, &result[0], size, 0, 0);
    result.resize(size - 1);
    return result;
}
} // namespace ui
} // namespace exam
