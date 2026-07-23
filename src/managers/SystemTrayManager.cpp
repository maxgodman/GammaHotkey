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
static bool s_iconAdded = false;

// Cached state icons, loaded once and reused to avoid leaking a GDI handle on every update.
static HICON s_iconOn = nullptr;
static HICON s_iconOff = nullptr;

extern HINSTANCE hInst;

namespace SystemTrayManager
{
    // Load (once) and return the tray icon for the given gamma state.
    static HICON GetStateIcon(const bool gammaEnabled)
    {
        HICON& cached = gammaEnabled ? s_iconOn : s_iconOff;
        if (!cached)
        {
            // The .ico resources carry both 16x16 and 32x32 frames; LoadImage selects the frame
            // nearest the requested size. Use the shell's small-icon metric, not the window's DPI
            // (GetSystemMetricsForDpi(GetDpiForWindow(...))): this icon is drawn by the shell in the
            // notification area, so the size that matters is the taskbar's, not that of whichever
            // monitor our window is on - values that differ on a mixed-DPI setup.
            const int iconSize = GetSystemMetrics(SM_CXSMICON);
            const UINT iconID = gammaEnabled ? IDI_ON : IDI_OFF;
            cached = (HICON)LoadImage(hInst, MAKEINTRESOURCE(iconID),
                                      IMAGE_ICON, iconSize, GetSystemMetrics(SM_CYSMICON),
                                      LR_DEFAULTCOLOR);
        }
        return cached;
    }

    void AddIcon(const HWND hwnd)
    {
        memset(&g_nid, 0, sizeof(g_nid));
        g_nid.cbSize = sizeof(NOTIFYICONDATA);
        g_nid.hWnd = hwnd;
        g_nid.uID = SystemTrayIDs::ID_ICON;
        g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        g_nid.uCallbackMessage = SystemTrayIDs::WM_ICON;
        g_nid.hIcon = GetStateIcon(App::state.IsGammaEnabled());
        wcsncpy_s(g_nid.szTip, App::GetStatusText().c_str(), _TRUNCATE);
        Shell_NotifyIcon(NIM_ADD, &g_nid);
        s_iconAdded = true;
    }

    UINT GetTaskbarCreatedMessage()
    {
        // RegisterWindowMessage returns the same atom for a given string across the whole system,
        // so register once and cache. Explorer broadcasts this to every top-level window (including
        // ours while it is hidden in the tray) when it recreates the taskbar.
        static const UINT s_taskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
        return s_taskbarCreated;
    }

    void RemoveIcon()
    {
        Shell_NotifyIcon(NIM_DELETE, &g_nid);
        s_iconAdded = false;

        // Release cached icons.
        if (s_iconOn)  { DestroyIcon(s_iconOn);  s_iconOn = nullptr; }
        if (s_iconOff) { DestroyIcon(s_iconOff); s_iconOff = nullptr; }
    }

    void UpdateIcon(const bool gammaEnabled)
    {
        // No-op if the tray icon hasn't been added yet, so this is safe to call from
        // any state change (e.g. during early initialization).
        if (!s_iconAdded)
            return;

        g_nid.hIcon = GetStateIcon(gammaEnabled);

        // Update tooltip text using shared status text function.
        // Truncates safely: szTip holds 128 wchars but a profile name can be longer.
        wcsncpy_s(g_nid.szTip, App::GetStatusText().c_str(), _TRUNCATE);

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

        AppendMenu(hMenu, MF_STRING, SystemTrayIDs::ID_TOGGLE, toggleText.c_str());

        AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);

        AppendMenu(hMenu, MF_STRING, SystemTrayIDs::ID_SHOW, L"Show");
        AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenu(hMenu, MF_STRING, SystemTrayIDs::ID_EXIT, L"Exit");

        SetForegroundWindow(hwnd);
        TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);

        // Required so the menu dismisses correctly when the user clicks elsewhere.
        PostMessage(hwnd, WM_NULL, 0, 0);

        DestroyMenu(hMenu);
    }
}
