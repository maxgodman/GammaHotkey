// Copyright (c) 2025 Max Godman

// Hotkey registration and handling.

/**
 * Hotkeys use the Win32 RegisterHotKey() API.
 *
 * HOW IT WORKS:
 * - RegisterHotKey() registers each bound key as a system-wide hotkey.
 * - When pressed, Windows posts a WM_HOTKEY message to our window, which WndProc dispatches.
 * - Registrations use MOD_NOREPEAT so holding a key does not fire repeatedly.
 *
 * WHY NOT A LOW-LEVEL KEYBOARD HOOK:
 * - A WH_KEYBOARD_LL hook sees every keystroke system-wide, which is the classic keylogger
 *   signature and a common cause of antivirus false positives.
 * - The only thing RegisterHotKey() cannot do that a hook can is fire on a bare modifier key
 *   (Alt/Ctrl/Shift/Win alone). We deliberately do not support binding those, see IsBindableKey().
 * - Everything else the app supports (letters, digits, function keys including F10, etc.)
 *   works with RegisterHotKey(). Only F12 is reserved by Windows (for the debugger).
 */

#pragma once

#include <windows.h>

namespace HotkeyManager
{
    /**
     * @brief Register all hotkeys with Windows.
     * @param hwnd Window handle that will receive WM_HOTKEY messages.
     */
    void RegisterAll(const HWND hwnd);

    /**
     * @brief Unregister all hotkeys.
     * @param hwnd Window handle that registered the hotkeys.
     */
    void UnregisterAll(const HWND hwnd);

    /**
     * @brief Handle a hotkey press, dispatched from a WM_HOTKEY message.
     * @param hotkeyId The hotkey ID (see HotkeyIDs in GammaHotkeyTypes.h).
     */
    void HandleHotkey(const int hotkeyId);

    /**
     * @brief Whether a virtual-key can be bound as a hotkey.
     *
     * Rejects bare modifier keys (which RegisterHotKey cannot fire on) and F12
     * (reserved by Windows). Use reasonForRejection to show the user why, if non-null.
     *
     * @param vk Virtual-key code to validate.
     * @param[out] reasonForRejection Optional. Set to a short explanation when the key is not bindable.
     * @return true if the key can be bound as a hotkey.
     */
    bool IsBindableKey(const UINT vk, const char** reasonForRejection = nullptr);
}
