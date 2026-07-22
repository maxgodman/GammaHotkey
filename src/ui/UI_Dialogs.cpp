// Copyright (c) 2025 Max Godman

// All popup/modal dialogs.

#include "framework.h"
#include "Resource.h"
#include "imgui.h"
#include "AppGlobals.h"
#include "UIGlobals.h"
#include "UI_Shared.h"
#include "ProfileManager.h"
#include "ConfigManager.h"
#include "HotkeyManager.h"
#include "StringUtils.h"

extern HINSTANCE hInst;

// Load a string-table entry and convert it to UTF-8 for ImGui.
static std::string LoadUIString(const UINT id)
{
    WCHAR buffer[256] = L"";
    const int len = LoadStringW(hInst, id, buffer, ARRAYSIZE(buffer));
    return StringUtils::WideToUTF8(std::wstring(buffer, len));
}

// Center a modal on the main window and keep it there every frame, so all dialogs open in the
// middle and can't be dragged off-center (a drag just snaps back). Call immediately before
// BeginPopupModal.
static void CenterNextModal()
{
    const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
}

void RenderHotkeyCaptureDialog()
{
    // Hotkey capture popup.
    if (UI::state.showHotkeyCapture)
    {
        ImGui::OpenPopup("Capture Hotkey");
        UI::state.showHotkeyCapture = false;
        UI::state.captureRejectReason.clear();

        // Suspend all hotkeys while capturing.
        if (!UI::state.hotkeySuspended)
        {
            HotkeyManager::UnregisterAll(App::mainWindow);
            UI::state.hotkeySuspended = true;
        }
    }
    
    CenterNextModal();
    if (ImGui::BeginPopupModal("Capture Hotkey", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        // Check if we should close the popup.
        if (UI::state.closeCapturePopup)
        {
            UI::state.closeCapturePopup = false;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::Text("Press any key...");
        ImGui::Separator();
        
        std::string typeStr;
        switch (UI::state.capturingHotkeyType)
        {
            case HotkeyCapture::TOGGLE:           typeStr = "Toggle On/Off"; break;
            case HotkeyCapture::PREVIOUS_PROFILE: typeStr = "Previous Profile"; break;
            case HotkeyCapture::NEXT_PROFILE:     typeStr = "Next Profile"; break;
            case HotkeyCapture::PROFILE:          typeStr = "Profile Hotkey"; break;
            default:                              typeStr = "Unknown"; break;
        }
        
        ImGui::Text("Capturing for: %s", typeStr.c_str());

        // Show why the last key press was rejected, if any.
        if (!UI::state.captureRejectReason.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.7f, 0.0f, 1.0f));
            ImGui::TextWrapped("%s", UI::state.captureRejectReason.c_str());
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();

        if (ImGui::Button("Clear", ImVec2(120, 0)))
        {
            // 0 clears the binding.
            SetHotkeyForCaptureTarget(0);

            ConfigManager::Save();
            HotkeyManager::RegisterAll(App::mainWindow);

            UI::state.capturingHotkeyType = HotkeyCapture::NONE;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            UI::state.capturingHotkeyType = HotkeyCapture::NONE;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    else if (UI::state.hotkeySuspended && UI::state.capturingHotkeyType == HotkeyCapture::NONE)
    {
        // Popup was closed, re-register hotkeys.
        HotkeyManager::RegisterAll(App::mainWindow);
        UI::state.hotkeySuspended = false;
    }
}

void RenderAboutDialog()
{
    // Displayed text lives in the string table (IDS_ABOUT_*), whose values are composed from
    // the VER_* macros in the .rc, so nothing is duplicated here. The title doubles as the
    // popup's ImGui ID, so load it once and pass the same bytes to OpenPopup and BeginPopupModal.
    const std::string title = LoadUIString(IDS_ABOUT_TITLE);

    if (UI::state.showAboutDialog)
    {
        ImGui::OpenPopup(title.c_str());
        UI::state.showAboutDialog = false;
    }

    CenterNextModal();
    if (ImGui::BeginPopupModal(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("%s", LoadUIString(IDS_ABOUT_VERSION).c_str());
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Text("%s", LoadUIString(IDS_ABOUT_DESCRIPTION).c_str());
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Text("%s", LoadUIString(IDS_ABOUT_COPYRIGHT).c_str());
        ImGui::Spacing();

        const float buttonWidth = 120.0f;
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - buttonWidth) * 0.5f);
        if (ImGui::Button(LoadUIString(IDS_ABOUT_OK).c_str(), ImVec2(buttonWidth, 0)))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void RenderHotkeyConflictDialog()
{
    if (UI::state.showHotkeyConflict)
    {
        ImGui::OpenPopup("Hotkey Conflict");
        UI::state.showHotkeyConflict = false;
    }
    
    CenterNextModal();
    if (ImGui::BeginPopupModal("Hotkey Conflict", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("This hotkey is already assigned to:");
        ImGui::Spacing();
        
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.7f, 0.0f, 1.0f));
        ImGui::Text("%s", UI::state.conflictDescription.c_str());
        ImGui::PopStyleColor();
        
        ImGui::Spacing();
        ImGui::Text("Do you want to reassign it?");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        if (ImGui::Button("Yes", ImVec2(120, 0)))
        {
            // Free the key from its previous owner, then bind it to the action being captured.
            ClearConflictingHotkey(UI::state.conflictingHotkey);
            SetHotkeyForCaptureTarget(UI::state.conflictingHotkey);

            ConfigManager::Save();
            HotkeyManager::RegisterAll(App::mainWindow);

            UI::state.capturingHotkeyType = HotkeyCapture::NONE;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("No", ImVec2(120, 0)))
        {
            UI::state.capturingHotkeyType = HotkeyCapture::NONE;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void RenderDeleteConfirmDialog()
{
    if (UI::state.showDeleteConfirm)
    {
        ImGui::OpenPopup("Delete Profile");
        UI::state.showDeleteConfirm = false;
    }
    
    CenterNextModal();
    if (ImGui::BeginPopupModal("Delete Profile", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (UI::state.deleteProfileIndex >= 0 && UI::state.deleteProfileIndex < (int)App::profiles.size())
        {
            ImGui::Text("Are you sure you want to delete:");
            ImGui::Spacing();
            
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.7f, 0.0f, 1.0f));
            ImGui::Text("%s", StringUtils::WideToUTF8(App::profiles[UI::state.deleteProfileIndex].name).c_str());
            ImGui::PopStyleColor();
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            if (ImGui::Button("Yes", ImVec2(120, 0)))
            {
                ProfileManager::DeleteProfile(UI::state.deleteProfileIndex);
                ConfigManager::Save();
                HotkeyManager::RegisterAll(App::mainWindow);

                SyncUIWithCurrentProfile();
                ImGui::CloseCurrentPopup();
                UI::state.deleteProfileIndex = -1;
            }
            
            ImGui::SameLine();
            
            if (ImGui::Button("No", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
                UI::state.deleteProfileIndex = -1;
            }
        }
        else
        {
            ImGui::CloseCurrentPopup();
            UI::state.deleteProfileIndex = -1;
        }
        
        ImGui::EndPopup();
    }
}

void RenderStartupShortcutErrorDialog()
{
    if (UI::state.showStartupShortcutError)
    {
        ImGui::OpenPopup("Startup Shortcut");
        UI::state.showStartupShortcutError = false;
    }

    // Pin the width to the (non-wrapped) header line so the wrapped body text has a known wrap
    // width from the first frame; height still auto-fits (the 0). Without a fixed width, TextWrapped
    // + auto-resize takes a couple of frames to settle the size, and the centering would place the
    // not-yet-settled window off-center on the first shown frame - a visible flicker.
    const char* header = "Couldn't update the \"Launch on Windows startup\" setting.";
    const float dialogWidth = ImGui::CalcTextSize(header).x + ImGui::GetStyle().WindowPadding.x * 2.0f;
    ImGui::SetNextWindowSize(ImVec2(dialogWidth, 0.0f), ImGuiCond_Always);

    CenterNextModal();
    if (ImGui::BeginPopupModal("Startup Shortcut", nullptr, ImGuiWindowFlags_NoResize))
    {
        ImGui::Text("%s", header);
        ImGui::Spacing();
        ImGui::TextWrapped("Windows wouldn't let the startup shortcut be changed, so the setting was "
                           "left as it was.");

        // Specific reason (with HRESULT) when we have one; the message above stands alone if not.
        if (!UI::state.startupShortcutErrorDetail.empty())
        {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.7f, 0.0f, 1.0f));
            ImGui::TextWrapped("%s", UI::state.startupShortcutErrorDetail.c_str());
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        const float buttonWidth = 120.0f;
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - buttonWidth) * 0.5f);
        if (ImGui::Button("OK", ImVec2(buttonWidth, 0)))
        {
            UI::state.startupShortcutErrorDetail.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void RenderAllDialogs()
{
    RenderHotkeyCaptureDialog();
    RenderAboutDialog();
    RenderHotkeyConflictDialog();
    RenderDeleteConfirmDialog();
    RenderStartupShortcutErrorDialog();
}
