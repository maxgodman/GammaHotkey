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

#include <string>

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
     * @param errorDetail Optional; on failure, filled with a human-readable reason (with the
     *        underlying HRESULT). Left untouched on success.
     * @return true if the shortcut was written/removed successfully, false if it wasn't (in which
     *         case the on-disk state is unchanged and callers should re-sync from IsEnabled()).
     */
    bool SetEnabled(const bool enabled, std::wstring* errorDetail = nullptr);
}
