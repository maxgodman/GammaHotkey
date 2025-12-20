// Copyright (c) 2025 Max Godman

// Profile management operations.

#pragma once

#include <string>

namespace ProfileManager
{
    /**
     * @brief Find profile index by name.
     * @param[in] name Profile name to search for.
     * @return Index in App::profiles vector, or -1 if not found.
     */
    int FindByName(const std::wstring& name);
    
    /**
     * @brief Apply a profile by its index.
     * @param[in] index Index in App::profiles vector.
     */
    void ApplyByIndex(const int index);
    
    /**
     * @brief Apply a profile by its name.
     * @param[in] name Profile name to search for and apply.
     * @return true if profile found and applied, false otherwise.
     */
    bool ApplyByName(const std::wstring& name);
    
    /**
     * @brief Cycle to next or previous profile.
     * @param[in] direction 1 for next, -1 for previous.
     */
    void CycleProfile(const int direction);
    
    /**
     * @brief Deletes a profile, updating related variables as needed.
     * @param[in] index Index of profile to delete.
     */
    void DeleteProfile(const int index);
}
