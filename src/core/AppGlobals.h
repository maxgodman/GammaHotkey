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
    extern std::wstring lastSelectedProfileName;
    
    // Simple mode profile.
    extern Profile simpleProfile;
    
    // Runtime state.
    // @TODO: Move to AppState?
    extern bool gammaRampFailed;
    extern float lastRamp[GammaConstants::RAMP_SIZE];
    
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
     * @brief Gets the desired window size X (width) of the app.
     */
    int GetDesiredWindowSizeX();

    /**
     * @brief Gets the desired window size Y (height) of the app.
     */
    int GetDesiredWindowSizeY();

    /**
     * @brief Syncs the window size to the current state of the app.
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
