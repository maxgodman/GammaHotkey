// Copyright (c) 2025 Max Godman

#include "framework.h"
#include "PathUtils.h"
#include <shlobj.h>

namespace PathUtils
{
    std::wstring GetConfigPath()
    {
        // Get executable path.
        WCHAR exePath[MAX_PATH];
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        
        // Get directory and filename without extension.
        std::wstring fullPath(exePath);
        size_t lastSlash = fullPath.find_last_of(L"\\/");
        size_t lastDot = fullPath.find_last_of(L'.');
        
        // Build config path: same directory and name as exe, but .ini extension.
        std::wstring configPath;
        if (lastDot != std::wstring::npos && lastDot > lastSlash)
        {
            configPath = fullPath.substr(0, lastDot) + L".ini";
        }
        else
        {
            configPath = fullPath + L".ini";
        }
        
        return configPath;
    }
    
    std::wstring GetExecutablePath()
    {
        WCHAR exePath[MAX_PATH];
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        return std::wstring(exePath);
    }
    
    std::wstring GetStartupShortcutPath()
    {
        WCHAR startupPath[MAX_PATH];
        if (SHGetFolderPathW(nullptr, CSIDL_STARTUP, nullptr, 0, startupPath) != S_OK)
        {
            return L"";
        }
        
        return std::wstring(startupPath) + L"\\GammaHotkey.lnk";
    }
}
