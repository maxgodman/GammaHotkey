// Copyright (c) 2025 Max Godman

// System tray icon management.

#pragma once

#include <windows.h>

namespace SystemTrayManager
{
    /**
     * @brief Add icon to system tray.
     * @param[in] hwnd Window handle for tray icon callbacks
     */
    void AddIcon(const HWND hwnd);

    /**
     * @brief The registered "TaskbarCreated" message Explorer broadcasts when it (re)creates the
     *        taskbar, e.g. after an Explorer restart.
     *
     * The restart destroys our tray icon, so the WndProc compares each incoming message against
     * this and re-adds the icon (via AddIcon) on a match, keeping the app reachable from the tray.
     * The value is registered once and cached. Being a runtime-registered message (0xC000-0xFFFF),
     * it cannot be a compile-time switch case. Returns 0 only if registration failed.
     */
    UINT GetTaskbarCreatedMessage();

    /**
     * @brief Remove icon from system tray.
     */
    void RemoveIcon();
    
    /**
     * @brief Update system tray icon and tooltip based on gamma state.
     * @param[in] gammaEnabled true = show green (on), false = show red (off)
     */
    void UpdateIcon(const bool gammaEnabled);
    
    /**
     * @brief Show context menu at cursor position.
     * @param[in] hwnd Window handle for menu commands.
     */
    void ShowContextMenu(const HWND hwnd);
}
