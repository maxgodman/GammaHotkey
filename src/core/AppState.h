// Copyright (c) 2025 Max Godman

// Centralized application state.

#pragma once

class AppState
{
public:
    /**
     * @brief Set gamma enabled state, and dispatch updates required to reflect this change, e.g. update icons.
     * @param enabled true = on, false = off.
     */
    void SetGammaEnabled(const bool enabled);

    /**
     * @brief Gets the gamma enabled state.
     * @return true = gamma is enabled.
     */
    bool IsGammaEnabled() const { return m_gammaEnabled; }

    /**
     * @brief Set advanced mode enabled state.
     * @param enabled true = advanced mode, false = simple mode.
     */
    void SetAdvancedModeEnabled(const bool enabled);

    /**
     * @brief Gets the advanced mode enabled state.
     * @return true = advanced mode is enabled, false = simple mode is enabled.
     */
    bool IsAdvancedModeEnabled() const { return m_advancedModeEnabled; }

private:
    bool m_gammaEnabled = false;
    bool m_advancedModeEnabled = false; // Default to Simple mode.
};
