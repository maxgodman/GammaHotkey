// Copyright (c) 2025 Max Godman

// Shared data structures and constants for GammaHotkey.

#pragma once

#include <windows.h>
#include <string>

/**
 * @brief Profile containing gamma adjustment settings and hotkey binding.
 */
struct Profile
{
    std::wstring name;
    int brightness = 0;      // Range: -50 to 50
    float contrast = 1.0f;   // Range: 0.5 to 1.5
    float gamma = 1.0f;      // Range: 0.1 to 3.0
    UINT hotkey = 0;         // Virtual key code, 0 = none.
    
    Profile() = default;
    Profile(std::wstring n, int b, float c, float g, UINT h)
        : name(n), brightness(b), contrast(c), gamma(g), hotkey(h) {}
};

/**
 * @brief Information for display selection.
 */
struct DisplayEntry
{
    std::wstring deviceName;   // Internal device name (e.g. "\\\\.\\DISPLAY1").
    std::wstring friendlyName; // User friendly name (e.g. "Branded Monitor | Branded GPU").
};

namespace HotkeyIDs
{
    constexpr int TOGGLE = 1;
    constexpr int PREVIOUS_PROFILE = 2;
    constexpr int NEXT_PROFILE = 3;
    constexpr int PROFILE_BASE = 1000;
}

namespace TrayIDs
{
    constexpr UINT WM_TRAYICON = WM_USER + 100;
    constexpr UINT ID_TRAY_SHOW = 2100;
    constexpr UINT ID_TRAY_EXIT = 2101;
    constexpr UINT ID_TRAY_TOGGLE = 2102;
    constexpr UINT ICON_ID = 1;
}

namespace GammaConstants
{
    constexpr int RAMP_SIZE = 256;        // Gamma ramp has 256 entries (8-bit).
    constexpr int RAMP_MAX = 65535;       // Each entry is 16-bit (0-65535).
    constexpr int MAX_LOADSTRING = 100;
}
