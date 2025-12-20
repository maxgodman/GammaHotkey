// Copyright (c) 2025 Max Godman

// Windows startup shortcut management implementation.

#include "framework.h"
#include "StartupManager.h"
#include "PathUtils.h"
#include <shlobj.h>
#include <shobjidl.h>
#include <filesystem>

namespace StartupManager
{
    bool IsEnabled()
    {
        const std::wstring shortcutPath = PathUtils::GetStartupShortcutPath();
        return std::filesystem::exists(shortcutPath);
    }
    
    void SetEnabled(const bool enabled)
    {
        std::wstring shortcutPath = PathUtils::GetStartupShortcutPath();
        
        if (enabled)
        {
            // Create shortcut.
            IShellLink* pShellLink = nullptr;
            HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, 
                                         IID_IShellLink, (void**)&pShellLink);
            
            if (SUCCEEDED(hr))
            {
                const std::wstring exePath = PathUtils::GetExecutablePath();
                const std::wstring exeDir = std::filesystem::path(exePath).parent_path().wstring();
                
                pShellLink->SetPath(exePath.c_str());
                pShellLink->SetWorkingDirectory(exeDir.c_str());
                pShellLink->SetDescription(L"GammaHotkey - Display Gamma Control");
                
                IPersistFile* pPersistFile = nullptr;
                hr = pShellLink->QueryInterface(IID_IPersistFile, (void**)&pPersistFile);
                
                if (SUCCEEDED(hr))
                {
                    pPersistFile->Save(shortcutPath.c_str(), TRUE);
                    pPersistFile->Release();
                }
                
                pShellLink->Release();
            }
        }
        else
        {
            // Remove shortcut.
            if (std::filesystem::exists(shortcutPath))
            {
                std::filesystem::remove(shortcutPath);
            }
        }
    }
}
