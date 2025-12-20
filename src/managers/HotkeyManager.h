// Copyright (c) 2025 Max Godman

// Hotkey registration and handling.

/**
 * A low-level keyboard hook is used to consume all keypresses.
 * RegisterHotKey() would ideally be used, but it cannot register various system keys such as Alt and F10.
 * The keyboard hook can capture any key, this is the same approach used by other hotkey tools.
 *
 * HOW IT WORKS:
 * - SetWindowsHookEx(WH_KEYBOARD_LL) installs a system-wide keyboard hook.
 * - Windows calls our LowLevelKeyboardProc for every keystroke.
 * - We check if the key matches any registered hotkey.
 * - If match: Call HandleHotkey() and return 1 (consumes key and does not pass it on to other apps).
 * - If no match: CallNextHookEx() (passes key through to other apps).
 */

#pragma once

#include <windows.h>

namespace HotkeyManager
{
    /**
     * @brief Register all hotkeys.
     * @param hwnd Window handle to receive WM_HOTKEY messages.
     */
    void RegisterAll(const HWND hwnd);
    
    /**
     * @brief Unregister all hotkeys.
     * @param hwnd Window handle that registered the hotkeys.
     */
    void UnregisterAll(const HWND hwnd);
    
    /**
     * @brief Handle a hotkey press.
     * @param hotkeyId The hotkey ID from keyboard hook.
     */
    void HandleHotkey(const int hotkeyId);
}
