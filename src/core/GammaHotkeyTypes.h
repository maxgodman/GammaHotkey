// Copyright (c) 2025 Max Godman

// Shared data structures and constants for GammaHotkey.

#pragma once

#include <windows.h>
#include <string>

/**
 * @brief Value ranges the profile sliders permit for each adjustment.
 *
 * The Simple/Advanced sliders bind to these, and ConfigManager clamps a loaded
 * profile to them, so the UI and the on-disk config share one source of truth.
 */
namespace ProfileRange
{
    // Brightness is an integer offset; contrast and gamma are float multipliers.
    constexpr int BRIGHTNESS_MIN = -50;
    constexpr int BRIGHTNESS_MAX = 50;
    constexpr int BRIGHTNESS_DEFAULT = 0;

    constexpr float CONTRAST_MIN = 0.5f;
    constexpr float CONTRAST_MAX = 1.5f;
    constexpr float CONTRAST_DEFAULT = 1.0f;

    constexpr float GAMMA_MIN = 0.1f;
    constexpr float GAMMA_MAX = 3.0f;
    constexpr float GAMMA_DEFAULT = 1.0f;
}

/**
 * @brief Profile containing gamma adjustment settings and hotkey binding.
 */
struct Profile
{
    std::wstring name;
    int brightness = ProfileRange::BRIGHTNESS_DEFAULT;
    float contrast = ProfileRange::CONTRAST_DEFAULT;
    float gamma = ProfileRange::GAMMA_DEFAULT;
    UINT hotkey = 0;  // Virtual key code, 0 = none.
    
    Profile() = default;
    Profile(std::wstring n, int b, float c, float g, UINT h)
        : name(n), brightness(b), contrast(c), gamma(g), hotkey(h) {}
};

/**
 * @brief Information for display selection.
 */
struct DisplayEntry
{
    std::wstring deviceName;    // Internal device name (e.g. "\\\\.\\DISPLAY1").
    std::wstring friendlyName;  // User friendly name (e.g. "Branded Monitor | Branded GPU").
};

namespace HotkeyIDs
{
    constexpr int TOGGLE = 1;
    constexpr int PREVIOUS_PROFILE = 2;
    constexpr int NEXT_PROFILE = 3;
    constexpr int PROFILE_BASE = 1000;
}

/**
 * @brief Identifies which action the hotkey-capture dialog is currently binding.
 *
 * Stored in UIState::capturingHotkeyType. NONE means the app is not capturing.
 * This is transient UI state and is never persisted, so the underlying values are
 * unspecified; switch on the enumerators, never on raw integers.
 */
enum class HotkeyCapture
{
    NONE,
    TOGGLE,
    PREVIOUS_PROFILE,
    NEXT_PROFILE,
    PROFILE,
};

namespace SystemTrayIDs
{
    constexpr UINT ID_ICON = 1;
    constexpr UINT WM_ICON = WM_USER + 100;
    constexpr UINT ID_TOGGLE = 2100;
    constexpr UINT ID_SHOW = 2101;
    constexpr UINT ID_EXIT = 2102;
}

namespace AppConstants
{
    constexpr int MAX_LOADSTRING = 100;

    // Default window sizes in logical (96 DPI / 100% scaling) pixels. App::SyncWindowSizeToState
    // multiplies them by App::GetDpiScale() before handing them to SetWindowPos.
    constexpr int DEFAULT_SIMPLE_WINDOWSIZE_X = 450;
    constexpr int DEFAULT_SIMPLE_WINDOWSIZE_Y = 520;
    constexpr int DEFAULT_ADVANCED_WINDOWSIZE_X = 900;
    constexpr int DEFAULT_ADVANCED_WINDOWSIZE_Y = 660;
}

namespace UIConstants
{
    constexpr float TITLEBAR_HEIGHT = 32.0f;
    constexpr float CONTENT_PADDING_X = 12.0f;
    constexpr float CONTENT_PADDING_Y = 12.0f;
    constexpr float CHECKBOX_INNERSPACING = 8.0f;
}

namespace GammaConstants
{
    constexpr int RAMP_SIZE = 256;  // Gamma ramp has 256 entries (8-bit).
    constexpr int RAMP_MAX = 65535; // Each entry is 16-bit (0-65535).
}
