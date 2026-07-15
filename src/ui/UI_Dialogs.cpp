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
            HotkeyManager::UnregisterAll(App::mainWindow);
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
    // @TODO: Store the text content used here in resources?
    if (UI::state.showAboutDialog)
    {
        ImGui::OpenPopup("About " VER_PRODUCTNAME);
        UI::state.showAboutDialog = false;
    }
    
    if (ImGui::BeginPopupModal("About " VER_PRODUCTNAME, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Version " VER_PRODUCTVERSION_STR);
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Text(VER_FILEDESCRIPTION);
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Text(VER_LEGALCOPYRIGHT);
        ImGui::Spacing();
        
        const float buttonWidth = 120.0f;
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - buttonWidth) * 0.5f);
        if (ImGui::Button("OK", ImVec2(buttonWidth, 0)))
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
            HotkeyManager::UnregisterAll(App::mainWindow);
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

void RenderAllDialogs()
{
    // Only render popups if ImGui context is valid.
    // Without this, we may crash when switching between Simple and Advanced mode.
    // @TODO: Investigate this further.
    if (!ImGui::GetCurrentContext())
        return;

    RenderHotkeyCaptureDialog();
    RenderAboutDialog();
    RenderHotkeyConflictDialog();
    RenderDeleteConfirmDialog();
}
