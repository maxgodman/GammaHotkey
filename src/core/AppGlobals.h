// Copyright (c) 2025 Max Godman

// Centralized application globals.

#pragma once

#include "GammaHotkeyTypes.h"
#include "AppState.h"
#include <vector>

namespace App
{
    // Global application state.
    extern AppState state;

    // Window handles.
    extern HWND mainWindow;

    // Display management.
    extern std::vector<DisplayEntry> displays;
    extern int selectedDisplayIndex;

    // Profile management.
    extern std::vector<Profile> profiles;
    extern Profile workingProfile; // Current working profile, may have unsaved changes, etc.
    extern int selectedProfileIndex; // Which profile is selected (-1 = none selected, persists when gamma toggled).
    
    // Global hotkeys.
    extern UINT toggleHotkey;
    extern UINT nextProfileHotkey;
    extern UINT previousProfileHotkey;
    
    // Application Settings.
    extern bool loopProfiles;
    extern bool startMinimized;
    extern bool minimizeToTray;
    extern bool launchOnStartup;
    extern bool applyProfileOnLaunch;

    // Simple mode profile.
    extern Profile simpleProfile;
        
    /**
     * @brief Syncs the gamma to the current state of the app.
     * 
     * This will effectively update the selected display with the desired gamma ramp,
     * based on the current application state.
     * 
     * The intent is for input handlers to update state as desired, then call this to apply the changes.
     * This avoids each input handler directly applying various changes, instead they modify state,
     * then this function determines what must be done to apply the desired gamma, then applies it.
     */
    void SyncGammaToState();

    /**
     * @brief Toggle gamma on/off, apply the change, and refresh the UI.
     *
     * The single shared toggle action used by both the toggle hotkey and the tray menu
     * "Toggle" item, so the two paths behave identically.
     */
    void ToggleGamma();

    /**
     * @brief The DPI scale factor of the monitor the main window is on (1.0 at 96 DPI / 100%).
     *
     * The single source for scaling our own hardcoded pixel literals. The app is declared
     * Per-Monitor DPI-Aware V2 (see the manifest), so Windows does no bitmap scaling for us and
     * every absolute dimension in the code has to be multiplied by this. Window size, title bar,
     * the non-client hit-test rects and the per-frame drawing all derive from this one value so
     * they cannot drift apart.
     *
     * Note this is deliberately *not* for the ImGui `style.*` metrics (padding, spacing, scrollbar
     * size): those are already scaled by `style.ScaleAllSizes()` in ImGuiRenderer, so applying
     * this factor to them as well would double-scale them.
     */
    float GetDpiScale();

    /**
     * @brief Gets the desired window size X (width) of the app, in logical (96 DPI) pixels.
     *        SyncWindowSizeToState applies GetDpiScale() to it.
     */
    int GetDesiredWindowSizeX();

    /**
     * @brief Gets the desired window size Y (height) of the app, in logical (96 DPI) pixels.
     *        SyncWindowSizeToState applies GetDpiScale() to it.
     */
    int GetDesiredWindowSizeY();

    /**
     * @brief Syncs the window size to the current state of the app, DPI-scaled.
     */
    void SyncWindowSizeToState();

    /**
     * @brief Checks if we have a selected profile, effectively validating selectedProfileIndex against the profiles vector.
     * @return true if selectedProfileIndex can be used to obtain a profile from profiles vector.
     */
    bool HasSelectedProfile();

    /**
     * @brief Get status text for use in several places, such as: title bar, system tray, tooltip.
     * @return Wide string like "GammaHotkey - On (Profile Name)" or "GammaHotkey - Off"
     */
    std::wstring GetStatusText();
}
