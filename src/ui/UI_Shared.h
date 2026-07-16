// Copyright (c) 2025 Max Godman

// Shared UI state, functions, components and utilities.

#pragma once

#include "GammaHotkeyTypes.h" // HotkeyCapture

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
 * @brief Renders the Simple/Advanced mode toggle button, pinned to the top-right corner just below
 *        the title bar. Clicking it does not switch modes inline; it records a deferred request
 *        (UI::state.targetAdvancedMode / modeJustChanged) that RenderMainUI applies at the top of the
 *        next frame, where resizing the window and swap chain is safe.
 */
void RenderModeToggleButton();

/**
 * @brief Renders the title bar.
 */
void RenderTitleBar();

/**
 * @brief Renders gamma curve visualization.
 *
 * Its fixed pixel metrics (minimum canvas size, curve and grid/border line thickness) are scaled
 * by the window's DPI so the graph keeps its proportions on high-DPI monitors.
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

/**
 * @brief Bind the action currently being captured (UI::state.capturingHotkeyType) to a key.
 * @param vk Virtual-key code, or 0 to clear the binding.
 *
 * Writes only the matching binding plus the profile-hotkey display buffer. The caller is
 * still responsible for saving config, re-registering hotkeys and closing the capture popup.
 */
void SetHotkeyForCaptureTarget(const UINT vk);
