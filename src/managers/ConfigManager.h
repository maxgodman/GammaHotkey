// Copyright (c) 2025 Max Godman

// Configuration file loading and saving.

#pragma once

namespace ConfigManager
{
    /**
     * @brief Load configuration from ini file.
     * 
     * Loads profiles, hotkeys, and settings into App:: namespace.
     * 
     * @return true if the configuration was loaded successfully.
     */
    bool Load();
    
    /**
     * @brief Save configuration to ini file.
     * 
     * Saves all profiles, hotkeys, and settings.
     * Writes to a temp file first, then replaces existing config with the new config.
     * 
     * @return true if the configuration was saved successfully.
     */
    bool Save();
}
