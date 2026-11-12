#ifndef SMARTEXAM_WINHTTP_API_H
#define SMARTEXAM_WINHTTP_API_H
#include <windows.h>
#include <stdexcept>
namespace exam {
// Старый MinGW не содержит winhttp.h; функции доступны в системной DLL Windows.
struct WinHttpApi {
    HMODULE module;
    void *(WINAPI *Open)(LPCWSTR, DWORD, LPCWSTR, LPCWSTR, DWORD);
    BOOL(WINAPI *CloseHandle)(void *);
    BOOL(WINAPI *SetTimeouts)(void *, int, int, int, int);
    BOOL(WINAPI *SetOption)(void *, DWORD, void *, DWORD);
    void *(WINAPI *Connect)(void *, LPCWSTR, WORD, DWORD);
    void *(WINAPI *OpenRequest)(void *, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR *, DWORD);
    BOOL(WINAPI *SendRequest)(void *, LPCWSTR, DWORD, void *, DWORD, DWORD, DWORD_PTR);
    BOOL(WINAPI *ReceiveResponse)(void *, void *);
    BOOL(WINAPI *QueryHeaders)(void *, DWORD, LPCWSTR, void *, DWORD *, DWORD *);
    BOOL(WINAPI *ReadData)(void *, void *, DWORD, DWORD *);
    WinHttpApi() {
        wchar_t directory[MAX_PATH];
        if (!GetSystemDirectoryW(directory, MAX_PATH))
            throw std::runtime_error("Не найден системный каталог Windows");
        std::wstring path = std::wstring(directory) + L"\\winhttp.dll";
        module = LoadLibraryW(path.c_str());
        if (!module)
            throw std::runtime_error("Не удалось загрузить WinHTTP");
#define EXAM_HTTP_FUNCTION(name)                                                                   \
    name = reinterpret_cast<decltype(name)>(GetProcAddress(module, "WinHttp" #name));              \
    if (!name)                                                                                     \
    throw std::runtime_error("В Windows отсутствует WinHttp" #name)
        EXAM_HTTP_FUNCTION(Open);
        EXAM_HTTP_FUNCTION(CloseHandle);
        EXAM_HTTP_FUNCTION(SetTimeouts);
        EXAM_HTTP_FUNCTION(SetOption);
        EXAM_HTTP_FUNCTION(Connect);
        EXAM_HTTP_FUNCTION(OpenRequest);
        EXAM_HTTP_FUNCTION(SendRequest);
        EXAM_HTTP_FUNCTION(ReceiveResponse);
        EXAM_HTTP_FUNCTION(QueryHeaders);
        EXAM_HTTP_FUNCTION(ReadData);
#undef EXAM_HTTP_FUNCTION
    }
    ~WinHttpApi() {
        FreeLibrary(module);
    }
};
inline WinHttpApi &httpApi() {
    static WinHttpApi api;
    return api;
}
} // namespace exam
#endif
