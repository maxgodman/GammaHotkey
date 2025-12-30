// Copyright (c) 2025 Max Godman

// Advanced mode UI.

#include "framework.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "AppGlobals.h"
#include "UIGlobals.h"
#include "UI_Shared.h"
#include "GammaManager.h"
#include "ProfileManager.h"
#include "ConfigManager.h"
#include "HotkeyManager.h"
#include "StringUtils.h"

/**
 * @brief Helper for read-only hotkey display.
 */
static void RenderHotkeyDisplay(const char* label, const char* id, UINT hotkey, int captureType)
{
    const float labelWidth = 135.0f;
    const float buttonWidth = 50.0f;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;

    // Calculate input width: total minus label, button, and one spacing gap.
    const float contentWidth = ImGui::GetContentRegionAvail().x;
    const float inputWidth = contentWidth - labelWidth - buttonWidth - spacing;

    // Label.
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", label);

    // Input at fixed position from start.
    ImGui::SameLine(labelWidth + ImGui::GetStyle().WindowPadding.x);

    std::string keyName = StringUtils::WideToUTF8(StringUtils::VkToName(hotkey));
    char buf[64];
    strncpy_s(buf, keyName.c_str(), sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    ImGui::BeginDisabled();
    ImGui::SetNextItemWidth(inputWidth);
    ImGui::InputText(id, buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);
    ImGui::EndDisabled();

    // Button.
    ImGui::SameLine(0, spacing);
    char buttonId[32];
    snprintf(buttonId, sizeof(buttonId), "Set##%s", id);
    if (ImGui::Button(buttonId, ImVec2(buttonWidth, 0)))
    {
        UI::state.showHotkeyCapture = true;
        UI::state.capturingHotkeyType = captureType;
    }
}

static void SelectProfile(int index)
{
    App::selectedProfileIndex = index;
    App::workingProfile = App::profiles[index];
    App::state.SetGammaEnabled(true);

    strncpy_s(UI::state.profileNameBuffer, sizeof(UI::state.profileNameBuffer),
        StringUtils::WideToUTF8(App::profiles[index].name).c_str(), _TRUNCATE);

    if (App::profiles[index].hotkey != 0)
    {
        strncpy_s(UI::state.profileHotkeyBuffer, sizeof(UI::state.profileHotkeyBuffer),
            StringUtils::WideToUTF8(StringUtils::VkToName(App::profiles[index].hotkey)).c_str(),
            _TRUNCATE);
    }
    else
    {
        UI::state.profileHotkeyBuffer[0] = '\0';
    }

    GammaManager::ApplyProfile(App::workingProfile, App::selectedDisplayIndex);
}

static void MoveProfileUp(int index)
{
    if (index <= 0) return;

    HotkeyManager::UnregisterAll(App::mainWindow);
    std::swap(App::profiles[index], App::profiles[index - 1]);

    if (App::selectedProfileIndex == index)
        App::selectedProfileIndex = index - 1;
    else if (App::selectedProfileIndex == index - 1)
        App::selectedProfileIndex = index;

    ConfigManager::Save();
    HotkeyManager::RegisterAll(App::mainWindow);
}

static void MoveProfileDown(int index)
{
    if (index >= (int)App::profiles.size() - 1) return;

    HotkeyManager::UnregisterAll(App::mainWindow);
    std::swap(App::profiles[index], App::profiles[index + 1]);

    if (App::selectedProfileIndex == index)
        App::selectedProfileIndex = index + 1;
    else if (App::selectedProfileIndex == index + 1)
        App::selectedProfileIndex = index;

    ConfigManager::Save();
    HotkeyManager::RegisterAll(App::mainWindow);
}

void RenderAdvancedUI()
{
    const ImGuiIO& io = ImGui::GetIO();

    const ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

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

    RenderTitleBar();

    ImGui::SetCursorPosY(UIConstants::TITLEBAR_HEIGHT);

    const float contentHeight = io.DisplaySize.y - UIConstants::TITLEBAR_HEIGHT;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(UIConstants::CONTENT_PADDING_X, UIConstants::CONTENT_PADDING_Y));

    if (ImGui::BeginChild("MainContent", ImVec2(0, contentHeight)))
    {
        const float availableWidth = ImGui::GetContentRegionAvail().x;
        const float separatorWidth = 1.0f;
        const float columnWidth = (availableWidth - separatorWidth) / 2.0f;

        // Left column.
        if (ImGui::BeginChild("LeftColumn", ImVec2(columnWidth, 0), ImGuiChildFlags_AlwaysUseWindowPadding))
        {
            RenderDisplayComboBox();

            ImGui::Spacing();
            ImGui::Spacing();

            ImGui::Text("Profile Settings");
            ImGui::Separator();

            ImGui::Text("Profile Name:");
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::InputText("##ProfileName", UI::state.profileNameBuffer, sizeof(UI::state.profileNameBuffer));

            ImGui::Text("Profile Hotkey:");

            char hotkeyBuf[64];
            strncpy_s(hotkeyBuf, UI::state.profileHotkeyBuffer, sizeof(hotkeyBuf) - 1);
            hotkeyBuf[sizeof(hotkeyBuf) - 1] = '\0';

            const float buttonWidth = 50.0f;
            const float spacing = ImGui::GetStyle().ItemSpacing.x;

            ImGui::BeginDisabled();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - buttonWidth - spacing);
            ImGui::InputText("##ProfileHotkey", hotkeyBuf, sizeof(hotkeyBuf), ImGuiInputTextFlags_ReadOnly);
            ImGui::EndDisabled();

            ImGui::SameLine();

            if (ImGui::Button("Set##ProfileHotkey", ImVec2(buttonWidth, 0)))
            {
                UI::state.showHotkeyCapture = true;
                UI::state.capturingHotkeyType = 4;
            }

            ImGui::Spacing();

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

            const std::string profileName = UI::state.profileNameBuffer;
            int existingProfileIndex = -1;
            if (!profileName.empty())
            {
                existingProfileIndex = ProfileManager::FindByName(StringUtils::UTF8ToWide(profileName));
            }

            const bool editingExistingProfile = (existingProfileIndex >= 0);
            const bool canUndo = (App::selectedProfileIndex >= 0) && profileModified;
            const char* saveButtonText = editingExistingProfile ? "Save Changes" : "Save New Profile";

            bool canSave = false;
            if (editingExistingProfile)
            {
                canSave = profileModified;
            }
            else
            {
                canSave = !profileName.empty();
            }

            const float saveUndoButtonWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

            ImGui::BeginDisabled(!canSave);
            if (ImGui::Button(saveButtonText, ImVec2(saveUndoButtonWidth, 0)))
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
            if (ImGui::Button("Undo", ImVec2(saveUndoButtonWidth, 0)))
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

            // Profile list.
            if (ImGui::BeginChild("ProfileList", ImVec2(0, 200), ImGuiChildFlags_Borders))
            {
                for (int i = 0; i < (int)App::profiles.size(); ++i)
                {
                    ImGui::PushID(i);

                    const bool selected = (App::selectedProfileIndex == i);
                    const bool renaming = (UI::state.renamingProfileIndex == i);

                    if (renaming)
                    {
                        ImGui::SetNextItemWidth(-1);

                        ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll;

                        if (UI::state.renameNeedsFocus)
                        {
                            ImGui::SetKeyboardFocusHere();
                            UI::state.renameNeedsFocus = false;
                        }

                        bool commitRename = ImGui::InputText("##rename", UI::state.renameBuffer,
                            sizeof(UI::state.renameBuffer), flags);

                        bool cancelRename = ImGui::IsKeyPressed(ImGuiKey_Escape);
                        bool lostFocus = !ImGui::IsItemFocused() && !UI::state.renameNeedsFocus;

                        if (commitRename || (lostFocus && !cancelRename))
                        {
                            if (UI::state.renameBuffer[0] != '\0')
                            {
                                App::profiles[i].name = StringUtils::UTF8ToWide(UI::state.renameBuffer);
                                if (selected)
                                {
                                    App::workingProfile.name = App::profiles[i].name;
                                    strncpy_s(UI::state.profileNameBuffer, sizeof(UI::state.profileNameBuffer),
                                        UI::state.renameBuffer, _TRUNCATE);
                                }
                                ConfigManager::Save();
                            }
                            UI::state.renamingProfileIndex = -1;
                        }
                        else if (cancelRename)
                        {
                            UI::state.renamingProfileIndex = -1;
                        }
                    }
                    else
                    {
                        std::string display = StringUtils::WideToUTF8(App::profiles[i].name);
                        if (App::profiles[i].hotkey != 0)
                        {
                            display += "  -  " + StringUtils::WideToUTF8(StringUtils::VkToName(App::profiles[i].hotkey));
                        }

                        // Store item position before drawing.
                        const ImVec2 itemPos = ImGui::GetCursorScreenPos();
                        const float itemHeight = ImGui::GetTextLineHeightWithSpacing();
                        const float fullWidth = ImGui::GetContentRegionAvail().x;

                        // Draw selectable with reduced width to leave room for buttons.
                        if (ImGui::Selectable(display.c_str(), selected, 0, ImVec2(fullWidth - 75, 0)))
                        {
                            SelectProfile(i);
                        }

                        // Check for double-click to rename.
                        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                        {
                            UI::state.renamingProfileIndex = i;
                            strncpy_s(UI::state.renameBuffer, sizeof(UI::state.renameBuffer),
                                StringUtils::WideToUTF8(App::profiles[i].name).c_str(), _TRUNCATE);
                            UI::state.renameNeedsFocus = true;
                        }

                        // Check if row is hovered for showing buttons.
                        const ImVec2 rowMin = itemPos;
                        const ImVec2 rowMax = ImVec2(itemPos.x + fullWidth, itemPos.y + itemHeight);
                        const bool rowHovered = ImGui::IsMouseHoveringRect(rowMin, rowMax);

                        if (rowHovered)
                        {
                            ImGui::SameLine(fullWidth - 70);

                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 0.8f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));

                            ImGui::BeginDisabled(i == 0);
                            if (ImGui::SmallButton("^##up"))
                            {
                                MoveProfileUp(i);
                            }
                            ImGui::EndDisabled();

                            ImGui::SameLine(0, 2);

                            ImGui::BeginDisabled(i >= (int)App::profiles.size() - 1);
                            if (ImGui::SmallButton("v##down"))
                            {
                                MoveProfileDown(i);
                            }
                            ImGui::EndDisabled();

                            ImGui::SameLine(0, 2);

                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
                            if (ImGui::SmallButton("X##delete"))
                            {
                                UI::state.deleteProfileIndex = i;
                                UI::state.showDeleteConfirm = true;
                            }
                            ImGui::PopStyleColor(); // Delete button hover color.

                            ImGui::PopStyleColor(3); // Button colors.
                        }
                    }

                    ImGui::PopID();
                }
            }
            ImGui::EndChild(); // ProfileList.
        }
        ImGui::EndChild(); // LeftColumn.

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical, separatorWidth);
        ImGui::SameLine();

        // Right column.
        if (ImGui::BeginChild("RightColumn", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding))
        {
            ImGui::Text("Global Hotkeys");
            ImGui::Separator();

            RenderHotkeyDisplay("Toggle On/Off:", "##ToggleHotkey", App::toggleHotkey, 0);
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Hotkey to toggle gamma adjustments on/off");
            }

            RenderHotkeyDisplay("Previous Profile:", "##PrevHotkey", App::previousProfileHotkey, 2);
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Hotkey to switch to the previous profile in the list");
            }

            RenderHotkeyDisplay("Next Profile:", "##NextHotkey", App::nextProfileHotkey, 3);
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Hotkey to switch to the next profile in the list");
            }

            ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(UIConstants::CHECKBOX_INNERSPACING, ImGui::GetStyle().ItemInnerSpacing.y));
            ImGui::Checkbox("Wrap around profile list", &App::loopProfiles);
            ImGui::PopStyleVar();
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
        }
        ImGui::EndChild(); // RightColumn.
    }
    ImGui::EndChild(); // MainContent.

    ImGui::PopStyleVar(); // WindowPadding.

    RenderModeToggleButton();

    ImGui::End();
}
