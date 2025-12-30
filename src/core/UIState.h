// Copyright (c) 2025 Max Godman

// Centralized UI state.

#pragma once

#include <Windows.h>
#include <string>

class UIState
{
public:
    // Profile editing buffers.
    char profileNameBuffer[256] = "";
    char profileHotkeyBuffer[128] = "";
    char renameBuffer[256] = "";

    // Dialog state.
    // @TODO: A better way to manage displaying the different modals/dialogs, rather than individual flags for each of them?
    bool showHotkeyCapture = false;
    bool showAboutDialog = false;
    bool showHotkeyConflict = false;
    bool showDeleteConfirm = false;
    bool closeCapturePopup = false;

    int deleteProfileIndex = -1;
    int renamingProfileIndex = -1;
    bool renameNeedsFocus = false;

    int capturingHotkeyType = -1; // @TODO: Find all usage of this and handle this better, it is hardcoded and fragile.
    bool hotkeySuspended = false;
    UINT conflictingHotkey = 0;
    std::string conflictDescription = "";

    // Mode switching.
    bool modeJustChanged = false;
    bool targetAdvancedMode = false;
};
