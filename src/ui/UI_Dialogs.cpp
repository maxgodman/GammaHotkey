// Copyright (c) 2025 Max Godman

// All popup/modal dialogs.

#include "framework.h"
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
            case 0: typeStr = "Toggle On/Off"; break;
            case 2: typeStr = "Previous Profile"; break;
            case 3: typeStr = "Next Profile"; break;
            case 4: typeStr = "Profile Hotkey"; break;
            default: typeStr = "Unknown"; break;
        }
        
        ImGui::Text("Capturing for: %s", typeStr.c_str());
        ImGui::Spacing();
        
        if (ImGui::Button("Clear", ImVec2(120, 0)))
        {
            // Clear the hotkey by setting it to 0 directly.
            switch (UI::state.capturingHotkeyType)
            {
            case 0: // Toggle.
                App::toggleHotkey = 0;
                break;
            case 2: // Previous.
                App::previousProfileHotkey = 0;
                break;
            case 3: // Next.
                App::nextProfileHotkey = 0;
                break;
            case 4: // Profile
                if (App::selectedProfileIndex >= 0 && App::selectedProfileIndex < (int)App::profiles.size())
                {
                    // Existing profile: Clear in both array and working copy.
                    App::profiles[App::selectedProfileIndex].hotkey = 0;
                    App::workingProfile.hotkey = 0;
                }
                else
                {
                    // New profile: Clear in working copy only.
                    App::workingProfile.hotkey = 0;
                }
                UI::state.profileHotkeyBuffer[0] = '\0';  // Clear the display buffer.
                break;
            }
            
            ConfigManager::Save();
            HotkeyManager::UnregisterAll(App::mainWindow);
            HotkeyManager::RegisterAll(App::mainWindow);
            
            UI::state.capturingHotkeyType = -1;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            UI::state.capturingHotkeyType = -1;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
    else if (UI::state.hotkeySuspended && UI::state.capturingHotkeyType == -1)
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
        ImGui::OpenPopup("About GammaHotkey");
        UI::state.showAboutDialog = false;
    }
    
    if (ImGui::BeginPopupModal("About GammaHotkey", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("GammaHotkey");
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Text("Gamma Hotkey");
        ImGui::Text("Version 0.1");
        ImGui::Spacing();
        ImGui::Text("Adjust display brightness, contrast, and gamma");
        ImGui::Text("with profiles and global hotkeys.");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        if (ImGui::Button("OK", ImVec2(120, 0)))
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
            // Clear the conflicting hotkey first.
            ClearConflictingHotkey(UI::state.conflictingHotkey);
            
            // Apply the new hotkey directly, not using ApplyHotkeyChange() to avoid flag issue.
            // @TODO: Handle this better?
            switch (UI::state.capturingHotkeyType)
            {
            case 0: // Toggle.
                App::toggleHotkey = UI::state.conflictingHotkey;
                break;
            case 2: // Previous.
                App::previousProfileHotkey = UI::state.conflictingHotkey;
                break;
            case 3: // Next.
                App::nextProfileHotkey = UI::state.conflictingHotkey;
                break;
            case 4: // Profile.
                if (App::selectedProfileIndex >= 0 && App::selectedProfileIndex < (int)App::profiles.size())
                {
                    // Existing profile: Update both array and working copy.
                    App::profiles[App::selectedProfileIndex].hotkey = UI::state.conflictingHotkey;
                    App::workingProfile.hotkey = UI::state.conflictingHotkey;
                }
                else
                {
                    // New profile: Update working copy only.
                    App::workingProfile.hotkey = UI::state.conflictingHotkey;
                }
                strncpy_s(UI::state.profileHotkeyBuffer, sizeof(UI::state.profileHotkeyBuffer),
                         StringUtils::WideToUTF8(StringUtils::VkToName(UI::state.conflictingHotkey)).c_str(), _TRUNCATE);
                break;
            }
            
            ConfigManager::Save();
            HotkeyManager::UnregisterAll(App::mainWindow);
            HotkeyManager::RegisterAll(App::mainWindow);
            
            UI::state.capturingHotkeyType = -1;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("No", ImVec2(120, 0)))
        {
            UI::state.capturingHotkeyType = -1;
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
