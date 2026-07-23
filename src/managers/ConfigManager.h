// Copyright (c) 2025 Max Godman

// Configuration file loading and saving.

#pragma once

#include <string>

namespace ConfigManager
{
    /**
     * @brief Strip characters that would corrupt the ini round-trip from a profile name.
     *
     * Trims surrounding whitespace and replaces the ini-structural characters
     * (`\r \n \t [ ] = # ;`) with underscores, falling back to "Unnamed Profile" if nothing
     * is left.
     *
     * @return the sanitized name.
     */
    std::wstring SanitizeProfileName(const std::wstring& name);

    /**
     * @brief Load configuration from ini file.
     * 
     * Loads profiles, hotkeys, and settings, primarily into AppGlobals and AppState.
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
