// Copyright (c) 2025 Max Godman

// Advanced mode UI.

#include "framework.h"
#include "imgui.h"
#include "AppGlobals.h"
#include "UIGlobals.h"
#include "UI_Shared.h"
#include "GammaManager.h"
#include "ProfileManager.h"
#include "ConfigManager.h"
#include "HotkeyManager.h"
#include "StringUtils.h"

void RenderAdvancedUI()
{
    const ImGuiIO& io = ImGui::GetIO();
    
    // Fullscreen window without any default decorations.
    const ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                     ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    
    // Ensure window covers entire screen with no gaps.
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    
    if (!ImGui::Begin("GammaHotkey", nullptr, windowFlags))
    {
        ImGui::PopStyleVar(3);
        ImGui::End();
        return;
    }
    
    ImGui::PopStyleVar(3);
    
    // Title bar.
    RenderTitleBar();
    
    // Content area starts directly after title bar, with padding handled by child.
    ImGui::SetCursorPosY(UIConstants::TITLEBAR_HEIGHT);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 12));

    // Use remaining height for content area.
    const float contentHeight = io.DisplaySize.y - UIConstants::TITLEBAR_HEIGHT;
    ImGui::BeginChild("MainContent", ImVec2(0, contentHeight), false);
    
    // Two-column layout.
    ImGui::BeginChild("LeftColumn", ImVec2(430, 0), true);
    
    // Display Selection.
    RenderDisplayComboBox();
    
    ImGui::Spacing();
    ImGui::Spacing();
    
    ImGui::Text("Profile Settings");
    ImGui::Separator();
    
    ImGui::Text("Profile Name:");
    ImGui::InputText("##ProfileName", UI::state.profileNameBuffer, sizeof(UI::state.profileNameBuffer));
    
    ImGui::Text("Profile Hotkey:");
    ImGui::InputText("##ProfileHotkey", UI::state.profileHotkeyBuffer, sizeof(UI::state.profileHotkeyBuffer), ImGuiInputTextFlags_ReadOnly);
    ImGui::SameLine();
    if (ImGui::Button("Set##ProfileHotkey"))
    {
        UI::state.showHotkeyCapture = true;
        UI::state.capturingHotkeyType = 4;
    }
    
    ImGui::Spacing();
    
    // Sliders using reusable components.
    RenderBrightnessSlider(App::workingProfile, true);
    RenderContrastSlider(App::workingProfile, true);
    RenderGammaSlider(App::workingProfile, true);
    
    ImGui::Spacing();
    
    // Check if profile has been modified.
    bool profileModified = false;
    if (App::selectedProfileIndex >= 0 && App::selectedProfileIndex < (int)App::profiles.size())
    {
        const Profile& saved = App::profiles[App::selectedProfileIndex];
        profileModified = (App::workingProfile.brightness != saved.brightness ||
                          App::workingProfile.contrast != saved.contrast ||
                          App::workingProfile.gamma != saved.gamma);
    }
    
    // Determine save button state and text based on profile name.
    const std::string profileName = UI::state.profileNameBuffer;
    int existingProfileIndex = -1;
    if (!profileName.empty())
    {
        existingProfileIndex = ProfileManager::FindByName(StringUtils::UTF8ToWide(profileName));
    }
    
    // We are editing if name matches an existing profile.
    const bool editingExistingProfile = (existingProfileIndex >= 0);
    
    // Can undo only if we are editing an existing profile AND it has been modified.
    const bool canUndo = (App::selectedProfileIndex >= 0) && profileModified;
    
    const char* saveButtonText = editingExistingProfile ? "Save Changes" : "Save New Profile";
    
    // For Save button enable state:
    // - If editing existing: enable only if modified
    // - If new profile: enable if name is not empty
    bool canSave = false;
    if (editingExistingProfile)
    {
        canSave = profileModified;  // Only enable if there are changes to save
    }
    else
    {
        canSave = !profileName.empty();  // Enable if we have a name for the new profile
    }
    
    // Profile buttons with disabled states.
    ImGui::BeginDisabled(!canSave);
    if (ImGui::Button(saveButtonText, ImVec2(130, 0)))
    {
        App::workingProfile.name = StringUtils::UTF8ToWide(profileName);
        
        int idx = ProfileManager::FindByName(App::workingProfile.name);
        HotkeyManager::UnregisterAll(App::mainWindow);
        
        if (idx >= 0)
        {
            App::profiles[idx] = App::workingProfile;
            App::selectedProfileIndex = idx;
        }
        else
        {
            App::profiles.push_back(App::workingProfile);
            App::selectedProfileIndex = (int)App::profiles.size() - 1;
        }
        
        ConfigManager::Save();
        HotkeyManager::RegisterAll(App::mainWindow);
    }

    ImGui::EndDisabled();
    
    ImGui::SameLine();
    
    ImGui::BeginDisabled(!canUndo);
    if (ImGui::Button("Undo", ImVec2(130, 0)))
    {
        if (App::selectedProfileIndex >= 0)
        {
            App::workingProfile = App::profiles[App::selectedProfileIndex];
            GammaManager::ApplyProfile(App::workingProfile, App::selectedDisplayIndex);
        }
    }

    ImGui::EndDisabled();
    
    ImGui::Spacing();
    ImGui::Spacing();
    
    ImGui::Text("Saved Profiles");
    ImGui::Separator();
    
    // @TODO: A bunch of this isn't working correctly after making various changes.
    ImGui::BeginChild("ProfileList", ImVec2(0, 200), true);
    for (int i = 0; i < (int)App::profiles.size(); ++i)
    {
        std::string display = StringUtils::WideToUTF8(App::profiles[i].name);
        if (App::profiles[i].hotkey != 0)
        {
            display += "  -  " + StringUtils::WideToUTF8(StringUtils::VkToName(App::profiles[i].hotkey));
        }
        
        ImGui::PushID(i);
        
        const ImVec2 itemPos = ImGui::GetCursorScreenPos();
        const float itemWidth = ImGui::GetContentRegionAvail().x;
        const float itemHeight = ImGui::GetTextLineHeightWithSpacing();
        
        // Check if mouse is hovering over this item area.
        const ImVec2 mousePos = ImGui::GetMousePos();
        const bool hovered = (mousePos.x >= itemPos.x && mousePos.x <= itemPos.x + itemWidth &&
                         mousePos.y >= itemPos.y && mousePos.y <= itemPos.y + itemHeight);
        
        // Draw buttons first, so they are on top in the event handling order.
        bool buttonClicked = false;
        if (hovered)
        {
            float buttonX = itemPos.x + itemWidth - 70;
            ImGui::SetCursorScreenPos(ImVec2(buttonX, itemPos.y));
            
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
            
            // Up button.
            if (i > 0)
            {
                if (ImGui::SmallButton("^##up"))
                {
                    HotkeyManager::UnregisterAll(App::mainWindow);
                    std::swap(App::profiles[i], App::profiles[i - 1]);
                    
                    if (App::selectedProfileIndex == i) App::selectedProfileIndex = i - 1;
                    else if (App::selectedProfileIndex == i - 1) App::selectedProfileIndex = i;
                    
                    if (App::selectedProfileIndex == i) App::selectedProfileIndex = i - 1;
                    else if (App::selectedProfileIndex == i - 1) App::selectedProfileIndex = i;
                    
                    ConfigManager::Save();
                    HotkeyManager::RegisterAll(App::mainWindow);
                    buttonClicked = true;
                }
            }
            else
            {
                ImGui::BeginDisabled();
                ImGui::SmallButton("^##up");
                ImGui::EndDisabled();
            }
            
            ImGui::SameLine(0, 2);
            
            // Down button.
            if (i < (int)App::profiles.size() - 1)
            {
                if (ImGui::SmallButton("v##down"))
                {
                    HotkeyManager::UnregisterAll(App::mainWindow);
                    std::swap(App::profiles[i], App::profiles[i + 1]);
                    
                    if (App::selectedProfileIndex == i) App::selectedProfileIndex = i + 1;
                    else if (App::selectedProfileIndex == i + 1) App::selectedProfileIndex = i;
                    
                    if (App::selectedProfileIndex == i) App::selectedProfileIndex = i + 1;
                    else if (App::selectedProfileIndex == i + 1) App::selectedProfileIndex = i;
                    
                    ConfigManager::Save();
                    HotkeyManager::RegisterAll(App::mainWindow);
                    buttonClicked = true;
                }
            }
            else
            {
                ImGui::BeginDisabled();
                ImGui::SmallButton("v##down");
                ImGui::EndDisabled();
            }
            
            ImGui::SameLine(0, 2);
            
            // Delete button.
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
            if (ImGui::SmallButton("X##delete"))
            {
                UI::state.deleteProfileIndex = i;
                UI::state.showDeleteConfirm = true;
                buttonClicked = true;
            }
            ImGui::PopStyleColor(4);
        }
        
        // Now draw the selectable, but only trigger if button wasn't clicked.
        ImGui::SetCursorScreenPos(itemPos);
        
        // Use colored background for selection.
        if (App::selectedProfileIndex == i)
        {
            ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyleColorVec4(ImGuiCol_Header));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
        }
        
        // Check if this profile is being renamed.
        const bool renaming = (UI::state.renamingProfileIndex == i);
        
        if (renaming)
        {
            // Show editable text field.
            ImGui::SetKeyboardFocusHere();
            if (ImGui::InputText("##rename", UI::state.renameBuffer, sizeof(UI::state.renameBuffer),
                                ImGuiInputTextFlags_EnterReturnsTrue))
            {
                // Enter pressed, save if not empty.
                if (UI::state.renameBuffer[0] != '\0')
                {
                    App::profiles[i].name = StringUtils::UTF8ToWide(UI::state.renameBuffer);
                    // Update currentProfile name if this is the selected profile.
                    if (App::selectedProfileIndex == i)
                    {
                        App::workingProfile.name = App::profiles[i].name;
                        strncpy_s(UI::state.profileNameBuffer, sizeof(UI::state.profileNameBuffer), UI::state.renameBuffer, _TRUNCATE);
                    }
                    ConfigManager::Save();
                }
                UI::state.renamingProfileIndex = -1;
            }
            
            // Check for focus loss or Escape.
            if (!ImGui::IsItemActive() || ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                // Save on focus loss if not empty.
                if (UI::state.renameBuffer[0] != '\0' && !ImGui::IsKeyPressed(ImGuiKey_Escape))
                {
                    App::profiles[i].name = StringUtils::UTF8ToWide(UI::state.renameBuffer);
                    if (App::selectedProfileIndex == i)
                    {
                        App::workingProfile.name = App::profiles[i].name;
                        strncpy_s(UI::state.profileNameBuffer, sizeof(UI::state.profileNameBuffer), UI::state.renameBuffer, _TRUNCATE);
                    }
                    ConfigManager::Save();
                }
                UI::state.renamingProfileIndex = -1;
            }
        }
        else
        {
            // Normal selectable.
            const bool clicked = ImGui::Selectable(display.c_str(), App::selectedProfileIndex == i) && !buttonClicked;
            
            // Check for double click first (takes priority).
            if (clicked && ImGui::IsMouseDoubleClicked(0))
            {
                // Double click detected, start rename.
                UI::state.renamingProfileIndex = i;
                strncpy_s(UI::state.renameBuffer, sizeof(UI::state.renameBuffer), StringUtils::WideToUTF8(App::profiles[i].name).c_str(), _TRUNCATE);
            }
            else if (clicked && !ImGui::IsMouseDoubleClicked(0))
            {
                // Single click only, select profile.
                App::selectedProfileIndex = i;
                
                App::workingProfile = App::profiles[i];
                App::selectedProfileIndex = i;
                App::state.SetGammaEnabled(true);
                strncpy_s(UI::state.profileNameBuffer, sizeof(UI::state.profileNameBuffer), StringUtils::WideToUTF8(App::profiles[i].name).c_str(), _TRUNCATE);
                
                if (App::profiles[i].hotkey != 0)
                {
                    strncpy_s(UI::state.profileHotkeyBuffer, sizeof(UI::state.profileHotkeyBuffer), StringUtils::WideToUTF8(StringUtils::VkToName(App::profiles[i].hotkey)).c_str(), _TRUNCATE);
                }
                else
                {
                    UI::state.profileHotkeyBuffer[0] = '\0';
                }
                
                GammaManager::ApplyProfile(App::workingProfile, App::selectedDisplayIndex);
            }
        }
        
        if (App::selectedProfileIndex == i)
        {
            ImGui::PopStyleColor(2);
        }
        
        ImGui::PopID();
    }

    ImGui::EndChild();
    
    ImGui::EndChild(); // Left column.
    
    ImGui::SameLine();
    ImGui::BeginChild("RightColumn", ImVec2(0, 0), true);
    
    ImGui::Text("Global Hotkeys");
    ImGui::Separator();
    
    ImGui::Text("Toggle On/Off:");
    ImGui::SameLine(120);
    const std::string toggleKeyName = StringUtils::WideToUTF8(StringUtils::VkToName(App::toggleHotkey));
    ImGui::InputText("##ToggleHotkey", (char*)toggleKeyName.c_str(), toggleKeyName.size() + 1, ImGuiInputTextFlags_ReadOnly);
    ImGui::SameLine();
    if (ImGui::Button("Set##Toggle")) { UI::state.showHotkeyCapture = true; UI::state.capturingHotkeyType = 0; }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Hotkey to toggle gamma adjustments on/off");
    }
    
    ImGui::Text("Previous Profile:");
    ImGui::SameLine(120);
    const std::string prevKeyName = StringUtils::WideToUTF8(StringUtils::VkToName(App::previousProfileHotkey));
    ImGui::InputText("##PrevHotkey", (char*)prevKeyName.c_str(), prevKeyName.size() + 1, ImGuiInputTextFlags_ReadOnly);
    ImGui::SameLine();
    if (ImGui::Button("Set##Prev")) { UI::state.showHotkeyCapture = true; UI::state.capturingHotkeyType = 2; }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Hotkey to switch to the previous profile in the list");
    }
    
    ImGui::Text("Next Profile:");
    ImGui::SameLine(120);
    const std::string nextKeyName = StringUtils::WideToUTF8(StringUtils::VkToName(App::nextProfileHotkey));
    ImGui::InputText("##NextHotkey", (char*)nextKeyName.c_str(), nextKeyName.size() + 1, ImGuiInputTextFlags_ReadOnly);
    ImGui::SameLine();
    if (ImGui::Button("Set##Next")) { UI::state.showHotkeyCapture = true; UI::state.capturingHotkeyType = 3; }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Hotkey to switch to the next profile in the list");
    }
    
    ImGui::Checkbox("Wrap around profile list", &App::loopProfiles);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("When reaching the end of the list, cycle back to the beginning (and vice versa)");
    }
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        ConfigManager::Save();
    }
    
    ImGui::Spacing();
    ImGui::Spacing();
    
    ImGui::Text("Options");
    ImGui::Separator();
    
    RenderOptionsCheckboxes();
    
    ImGui::Spacing();
    ImGui::Spacing();
    
    ImGui::Text("Gamma Curve Preview");
    ImGui::Separator();
    
    if (App::gammaRampFailed)
    {
        ImGui::TextColored(ImVec4(0.86f, 0.21f, 0.27f, 1.0f), "Warning: Values too extreme!");
    }
    
    DrawGammaCurve();
    
    ImGui::EndChild(); // Right column.
    
    ImGui::EndChild(); // MainContent.
    ImGui::PopStyleVar();
    
    RenderModeToggleButton();
    
    ImGui::End(); // GammaHotkey main window.
}
