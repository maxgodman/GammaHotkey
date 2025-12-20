// Copyright (c) 2025 Max Godman

// Manages icons across the application.

#pragma once

#include <windows.h>

namespace IconManager
{
    /**
     * @brief Update all icons based on gamma enabled state.
     * @param[in] gammaEnabled true = green (on), false = red (off).
     */
    void UpdateAllIcons(const bool gammaEnabled);
    
    /**
     * @brief Initialize icons on startup.
     * Sets initial icon state based on gammaEnabled.
     */
    void Initialize(const HWND hwnd, const bool gammaEnabled);
}
