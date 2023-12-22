#include "Paths.h"

#include <codecvt>
#include <locale>
#include <windows.h>

std::string FPaths::GetProjectDirectory()
{
    wchar_t szPath[MAX_PATH];
    GetModuleFileNameW( NULL, szPath, MAX_PATH );
    std::wstring wideStr(szPath);
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::string exePath = converter.to_bytes(wideStr);
    return exePath.substr(0, exePath.find_last_of("\\/")); 
}

std::string FPaths::GetContentDirectory()
{
    return GetProjectDirectory() + "\\Content";
}
