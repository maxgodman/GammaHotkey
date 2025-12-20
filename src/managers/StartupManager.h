// Copyright (c) 2025 Max Godman

// Windows startup shortcut management.
// We manage launching the application on Windows startup using a startup shortcut.
// This seems to be the least intrusive way to handle this for a portable application.
// Worst case scenario:
// - User checks "Launch on Windows startup", a shortcut is created.
// - They delete/rename/move the executable, without first unchecking the option.
// - The startup shortcut is left behind and won't be cleaned up.
// This isn't ideal, but it is harmless to the system.

#pragma once

namespace StartupManager
{
    /**
     * @brief Check if application is set to launch on Windows startup.
     * @return true if startup shortcut exists.
     */
    bool IsEnabled();
    
    /**
     * @brief Enable or disable launch on Windows startup.
     * @param enabled true to enable, false to disable.
     */
    void SetEnabled(const bool enabled);
}
