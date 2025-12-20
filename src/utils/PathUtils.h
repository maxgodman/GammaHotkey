// Copyright (c) 2025 Max Godman

// File path and directory utilities.

#pragma once

#include <string>

namespace PathUtils
{
    /**
     * @brief Get the full path to the configuration file.
     * @return Path to ini file, expected to be alongside the executable with matching name.
     * e.g. GammaHotkey.ini
     */
    std::wstring GetConfigPath();
    
    /**
     * @brief Get the full path to the executable.
     * @return Path to the running executable, e.g. GammaHotkey.exe
     */
    std::wstring GetExecutablePath();
    
    /**
     * @brief Get the path where the startup shortcut should be placed.
     * @return Path to startup folder shortcut location.
     */
    std::wstring GetStartupShortcutPath();
}
