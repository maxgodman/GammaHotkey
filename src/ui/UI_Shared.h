// Copyright (c) 2025 Max Godman

// Shared UI state, functions, components and utilities.

#pragma once

#include "GammaHotkeyTypes.h"

namespace UIConstants
{
    constexpr float TITLEBAR_HEIGHT = 32.0f;
}

/**
 * @brief Renders the Display selection combo box.
 */
void RenderDisplayComboBox();

// Render option checkboxes (minimize to tray, launch on startup, etc.)
void RenderOptionsCheckboxes();

// Render adjustable sliders.
void RenderBrightnessSlider(Profile& profile, const bool advancedMode);
void RenderContrastSlider(Profile& profile, bool advancedMode);
void RenderGammaSlider(Profile& profile, bool advancedMode);

/**
 * @brief Renders the button used to toggle between Simple and Advanced UI modes.
 * @TODO: Improve/replace this, not keen on this, but it works.
 */
void RenderModeToggleButton();

/**
 * @brief Renders the title bar.
 */
void RenderTitleBar();

/**
 * @brief Renders gamma curve visualization.
 * @TODO: This does not respect DPI changes.
 */
void DrawGammaCurve();

/**
 * @brief Apply custom ImGui styling.
 */
void ApplyImGuiStyle();

/**
 * @brief Sync UI state with currently selected profile.
 */
void SyncUIWithCurrentProfile();

/**
 * @brief Handle hotkey capture from keyboard hook.
 */
void OnHotkeyCapture(const UINT vk);

/**
 * @brief Clear any hotkeys that match the given virtual key.
 * @param vk Virtual-key code (e.g. VK_F1, VK_CONTROL).
 */
void ClearConflictingHotkey(const UINT vk);
