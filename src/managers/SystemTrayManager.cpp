// Copyright (c) 2025 Max Godman

#include "framework.h"
#include "SystemTrayManager.h"
#include "GammaHotkeyTypes.h"
#include "AppGlobals.h"
#include "StringUtils.h"
#include "Resource.h"
#include <shellapi.h>

// Global tray icon data.
static NOTIFYICONDATA g_nid = {};
extern HINSTANCE hInst;

namespace SystemTrayManager
{
    void AddIcon(const HWND hwnd)
    {
        memset(&g_nid, 0, sizeof(g_nid));
        g_nid.cbSize = sizeof(NOTIFYICONDATA);
        g_nid.hWnd = hwnd;
        g_nid.uID = TrayIDs::ICON_ID;
        g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        g_nid.uCallbackMessage = TrayIDs::WM_TRAYICON;
        g_nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_GAMMAHOTKEY));
        wcscpy_s(g_nid.szTip, L"GammaHotkey");
        Shell_NotifyIcon(NIM_ADD, &g_nid);
    }
    
    void RemoveIcon()
    {
        Shell_NotifyIcon(NIM_DELETE, &g_nid);
    }
    
    void UpdateIcon(const bool gammaEnabled)
    {
        // Update icon based on gamma state.
        const UINT iconID = gammaEnabled ? IDI_ON : IDI_OFF;
        
        // Load icon with specific size for tray (16x16).
        g_nid.hIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(iconID),
                                       IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
        
        // Update tooltip text using shared status text function.
        const std::wstring tooltipText = App::GetStatusText();
        wcscpy_s(g_nid.szTip, tooltipText.c_str());
        
        Shell_NotifyIcon(NIM_MODIFY, &g_nid);
    }
    
    void ShowContextMenu(const HWND hwnd)
    {
        POINT pt;
        GetCursorPos(&pt);
        
        const HMENU hMenu = CreatePopupMenu();
        
        // Add header (disabled/grayed out, uses shared status text).
        const std::wstring headerText = App::GetStatusText();
        AppendMenu(hMenu, MF_STRING | MF_DISABLED | MF_GRAYED, 0, headerText.c_str());
        
        AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
        
        // Add toggle option with hotkey.
        std::wstring toggleText;
        if (App::state.IsGammaEnabled())
        {
            toggleText = L"Toggle Off";
        }
        else
        {
            toggleText = L"Toggle On";
        }
        
        // Append hotkey if one is set.
        if (App::toggleHotkey != 0)
        {
            std::wstring hotkeyName = StringUtils::VkToName(App::toggleHotkey);
            toggleText += L" (" + hotkeyName + L")";
        }
        
        AppendMenu(hMenu, MF_STRING, TrayIDs::ID_TRAY_TOGGLE, toggleText.c_str());
        
        AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
        
        AppendMenu(hMenu, MF_STRING, TrayIDs::ID_TRAY_SHOW, L"Show");
        AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenu(hMenu, MF_STRING, TrayIDs::ID_TRAY_EXIT, L"Exit");
        
        SetForegroundWindow(hwnd);
        TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
        DestroyMenu(hMenu);
    }
}
