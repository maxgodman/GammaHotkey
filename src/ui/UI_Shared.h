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
 * @brief Width for a button carrying @p label: its nominal design width scaled for DPI, but never
 *        narrower than the label actually needs.
 * @param label Button text, measured with the current font. Any "##id" suffix must be stripped
 *        first - only pass the part that is drawn.
 * @param nominalWidth The button's design width in logical (96 DPI) pixels.
 *
 * A plain literal breaks in two independent ways: the DPI factor scales the text but not the
 * literal, and so does a change of UI font. Both showed up as clipped button labels. Scaling the
 * nominal width fixes the first; the measured floor fixes the second and guarantees the label
 * fits whatever happens. At 100% with a label that fits, this returns the nominal width, so
 * buttons keep exactly the proportions they were designed with.
 */
float GetScaledButtonWidth(const char* label, const float nominalWidth);

/**
 * @brief The title bar height in physical pixels: UIConstants::TITLEBAR_HEIGHT scaled by
 *        App::GetDpiScale().
 *
 * Three things have to agree on this number, and they live in three different files: the bar
 * RenderTitleBar draws, the non-client hit-test rects it publishes for WM_NCHITTEST, and the
 * offset at which each mode starts laying out its content. They all read it from here rather than
 * each multiplying the constant themselves, so a caption button can never be drawn somewhere
 * other than where it is clickable.
 */
float GetTitleBarHeight();

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
 * @brief Handle key capture for the "set hotkey" popup (from WM_KEYDOWN).
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
