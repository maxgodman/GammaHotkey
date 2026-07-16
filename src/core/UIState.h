// Copyright (c) 2025 Max Godman

// Centralized UI state.

#pragma once

#include <Windows.h>
#include <string>

#include "GammaHotkeyTypes.h"

class UIState
{
public:
    // Profile editing buffers.
    char profileNameBuffer[256] = "";
    char profileHotkeyBuffer[128] = "";
    char renameBuffer[256] = "";

    // Dialog state.
    // Each bool is a one-shot "open this popup next frame" request: a caller sets it, the matching
    // Render*Dialog() calls ImGui::OpenPopup() and clears it. ImGui's popup stack owns which modal
    // is open, so these per-frame flags stay simpler than a single "active dialog" enum.
    bool showHotkeyCapture = false;
    bool showAboutDialog = false;
    bool showHotkeyConflict = false;
    bool showDeleteConfirm = false;
    bool closeCapturePopup = false;

    int deleteProfileIndex = -1;
    int renamingProfileIndex = -1;
    bool renameNeedsFocus = false;

    HotkeyCapture capturingHotkeyType = HotkeyCapture::NONE;
    bool hotkeySuspended = false;
    UINT conflictingHotkey = 0;
    std::string conflictDescription = "";
    std::string captureRejectReason = ""; // Shown in the capture dialog when an unbindable key is pressed.

    // Mode switching.
    bool modeJustChanged = false;
    bool targetAdvancedMode = false;
};
