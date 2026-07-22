// Copyright (c) 2025 Max Godman

// Windows startup shortcut management implementation.

#include "framework.h"
#include "StartupManager.h"
#include "PathUtils.h"
#include <shlobj.h>
#include <shobjidl.h>
#include <filesystem>
#include <system_error>

namespace
{
    // Format an HRESULT as "(0x........): <system message>" for display.
    std::wstring DescribeHResult(const HRESULT hr)
    {
        wchar_t code[16] = L"";
        swprintf_s(code, L"0x%08X", static_cast<unsigned int>(hr));

        std::wstring detail = L"(";
        detail += code;
        detail += L")";

        LPWSTR sysMsg = nullptr;
        const DWORD len = FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, static_cast<DWORD>(hr), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPWSTR>(&sysMsg), 0, nullptr);

        if (len > 0 && sysMsg)
        {
            std::wstring msg(sysMsg, len);
            // FormatMessage appends a trailing CR/LF (and sometimes a period/space); trim it.
            while (!msg.empty() && (msg.back() == L'\r' || msg.back() == L'\n' || msg.back() == L' '))
                msg.pop_back();
            if (!msg.empty())
            {
                detail += L": ";
                detail += msg;
            }
        }

        if (sysMsg)
            LocalFree(sysMsg);

        return detail;
    }
}

namespace StartupManager
{
    bool IsEnabled()
    {
        const std::wstring shortcutPath = PathUtils::GetStartupShortcutPath();
        std::error_code ec;
        return std::filesystem::exists(shortcutPath, ec);
    }

    bool SetEnabled(const bool enabled, std::wstring* errorDetail)
    {
        std::wstring shortcutPath = PathUtils::GetStartupShortcutPath();

        if (enabled)
        {
            IShellLink* pShellLink = nullptr;
            HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                          IID_IShellLink, reinterpret_cast<void**>(&pShellLink));
            if (FAILED(hr))
            {
                if (errorDetail) *errorDetail = L"Could not create the shortcut object " + DescribeHResult(hr);
                return false;
            }

            const std::wstring exePath = PathUtils::GetExecutablePath();
            const std::wstring exeDir = std::filesystem::path(exePath).parent_path().wstring();

            pShellLink->SetPath(exePath.c_str());
            pShellLink->SetWorkingDirectory(exeDir.c_str());
            pShellLink->SetDescription(L"GammaHotkey - Display Gamma Control");

            IPersistFile* pPersistFile = nullptr;
            hr = pShellLink->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&pPersistFile));
            if (FAILED(hr))
            {
                if (errorDetail) *errorDetail = L"Could not access the shortcut file interface " + DescribeHResult(hr);
                pShellLink->Release();
                return false;
            }

            hr = pPersistFile->Save(shortcutPath.c_str(), TRUE);
            pPersistFile->Release();
            pShellLink->Release();

            if (FAILED(hr))
            {
                if (errorDetail) *errorDetail = L"Could not save the startup shortcut " + DescribeHResult(hr);
                return false;
            }

            return true;
        }
        else
        {
            // Remove shortcut.
            std::error_code ec;
            if (std::filesystem::exists(shortcutPath, ec))
            {
                std::filesystem::remove(shortcutPath, ec);
                if (ec)
                {
                    if (errorDetail)
                    {
                        wchar_t code[16] = L"";
                        swprintf_s(code, L"(%d)", ec.value());
                        *errorDetail = std::wstring(L"Could not remove the startup shortcut ") + code + L".";
                    }
                    return false;
                }
            }

            return true;
        }
    }
}
