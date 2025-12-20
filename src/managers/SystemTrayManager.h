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
