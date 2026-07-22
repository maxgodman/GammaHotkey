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
    bool showStartupShortcutError = false;
    bool closeCapturePopup = false;

    int deleteProfileIndex = -1;
    int renamingProfileIndex = -1;
    bool renameNeedsFocus = false;

    HotkeyCapture capturingHotkeyType = HotkeyCapture::NONE;
    bool hotkeySuspended = false;
    UINT conflictingHotkey = 0;
    std::string conflictDescription = "";
    std::string captureRejectReason = ""; // Shown in the capture dialog when an unbindable key is pressed.
    std::string startupShortcutErrorDetail = ""; // Shown in the startup-shortcut error dialog; may be empty (no HRESULT detail).

    // Mode switching.
    bool modeJustChanged = false;
    bool targetAdvancedMode = false;

    // Title-bar hit-test regions, published by RenderTitleBar each frame and read by WM_NCHITTEST,
    // which runs outside the ImGui render pass and so cannot query widget rects directly. Coordinates
    // are client-space pixels; for the main window (pinned at 0,0, sized to the client area) those
    // equal ImGui screen coordinates. Client x in [0, dragRight) is the draggable caption (reported
    // HTCAPTION, so Windows runs the move loop behind Aero Snap, taskbar peek, window-shake and
    // drag-off-maximize); [dragRight, width) is the button cluster (HTCLIENT, so ImGui gets the clicks).
    //
    // The minimize/maximize/close buttons are custom-drawn as non-client caption buttons:
    // WM_NCHITTEST reports each as HTMINBUTTON/HTMAXBUTTON/HTCLOSE (so the maximize button drives
    // the Windows 11 snap-layouts flyout) and the WndProc runs the action on click. Their rects are
    // published here for the hit test and take priority over the caption and resize border.
    struct TitleBarHitRegions
    {
        bool valid = false;    // Stays false until the first frame publishes real values.
        int captionBottom = 0; // Caption band spans client y in [0, captionBottom).
        int dragRight = 0;     // Draggable caption spans client x in [0, dragRight).
        RECT minButton = {};   // Client-space rects of the three window-control buttons.
        RECT maxButton = {};
        RECT closeButton = {};
    };
    TitleBarHitRegions titleBar;
};
