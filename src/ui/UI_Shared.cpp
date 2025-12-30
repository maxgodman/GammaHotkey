// Copyright (c) 2025 Max Godman

// Shared UI components and utilities.

#include "framework.h"
#include "UI_Shared.h"
#include "imgui.h"
#include "AppGlobals.h"
#include "UIGlobals.h"
#include "GammaManager.h"
#include "ConfigManager.h"
#include "StartupManager.h"
#include "StringUtils.h"
#include <string>

/**
 * @brief Check if hotkey is already in use.
 * @param vk Virtual key to check.
 * @return Returns conflicting hotkey as friendly string, empty if no conflict.
 */
static std::string CheckHotkeyConflict(const UINT vk)
{
    // @TODO:   This isn't great, handle this better.
    //          Note this doesn't support identifying that the key is already bound to the target action.
    //          Using this we will always get a conflict prompt even if we are just trying to re-assign the same key.

    if (vk == 0) return "";
    
    if (App::toggleHotkey == vk) return "Toggle On/Off";
    if (App::previousProfileHotkey == vk) return "Previous Profile";
    if (App::nextProfileHotkey == vk) return "Next Profile";
    
    for (size_t i = 0; i < App::profiles.size(); ++i)
    {
        if (App::profiles[i].hotkey == vk)
        {
            return "Profile: " + StringUtils::WideToUTF8(App::profiles[i].name);
        }
    }

    return ""; // No conflict.
}

/**
 * @brief Apply hotkey change after capture.
 */
static void ApplyHotkeyChange(const UINT vk)
{
    // Check for conflicts.
    const std::string conflict = CheckHotkeyConflict(vk);
    if (!conflict.empty())
    {
        UI::state.showHotkeyConflict = true;
        UI::state.conflictingHotkey = vk;
        UI::state.conflictDescription = conflict;
        return;
    }
    
    // Apply the new hotkey (or clear it if vk is 0).
    switch (UI::state.capturingHotkeyType)
    {
    case 0: // Toggle.
        App::toggleHotkey = vk;
        break;
    case 2: // Previous.
        App::previousProfileHotkey = vk;
        break;
    case 3: // Next.
        App::nextProfileHotkey = vk;
        break;
    case 4: // Profile.
        // For new profiles (selectedProfileIndex == -1): Store in workingProfile temporarily.
        // For existing profiles: Update both profiles array and workingProfile.
        // When user saves, workingProfile.hotkey gets committed to profiles array.
        
        if (App::selectedProfileIndex >= 0 && App::selectedProfileIndex < (int)App::profiles.size())
        {
            // Existing profile: Update saved profile and working copy.
            App::profiles[App::selectedProfileIndex].hotkey = vk;
            App::workingProfile.hotkey = vk;
        }
        else
        {
            // New profile (not yet saved): Update working copy only.
            // When user clicks "Save New Profile", this will be committed.
            App::workingProfile.hotkey = vk;
        }
        
        // Update display buffer (works for both new and existing profiles).
        strncpy_s(UI::state.profileHotkeyBuffer, sizeof(UI::state.profileHotkeyBuffer),
                 StringUtils::WideToUTF8(StringUtils::VkToName(vk)).c_str(), _TRUNCATE);
        break;
    }
    
    // Always save when assigning hotkeys.
    ConfigManager::Save();
    
    // Mark that we're done capturing, this will trigger hotkey re-registration.
    UI::state.capturingHotkeyType = -1;
    UI::state.closeCapturePopup = true;  // Request popup closure.
}

void RenderDisplayComboBox()
{
    ImGui::Text("Display");
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    if (ImGui::BeginCombo("##Display", App::displays.empty() ? "No displays" :
        StringUtils::WideToUTF8(App::displays[App::selectedDisplayIndex].friendlyName).c_str()))
    {
        for (int i = 0; i < (int)App::displays.size(); ++i)
        {
            const bool selected = (App::selectedDisplayIndex == i);
            if (ImGui::Selectable(StringUtils::WideToUTF8(App::displays[i].friendlyName).c_str(), selected))
            {
                // Reset gamma on the old display before switching.
                if (App::selectedDisplayIndex != i)
                {
                    GammaManager::ResetDisplay(App::selectedDisplayIndex);
                }

                // Switch to new display.
                App::selectedDisplayIndex = i;

                // Sync post display change.
                App::SyncGammaToState();
                ConfigManager::Save();  // Save selected display.
            }

            if (selected)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Select which display to adjust");
    }
}

void RenderOptionsCheckboxes()
{
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(UIConstants::CHECKBOX_INNERSPACING, ImGui::GetStyle().ItemInnerSpacing.y));
    if (ImGui::Checkbox("Run in background when closed", &App::minimizeToTray))
    {
        ConfigManager::Save();
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Minimize to system tray instead of closing when you click the X button");
    }
    
    if (ImGui::Checkbox("Run in background when launched", &App::startMinimized))
    {
        ConfigManager::Save();
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Start minimized to system tray instead of showing the window");
    }
    
    if (ImGui::Checkbox("Toggle on when launched", &App::applyProfileOnLaunch))
    {
        ConfigManager::Save();
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Automatically toggle gamma on and apply profile when the application starts");
    }
    
    if (ImGui::Checkbox("Launch on Windows startup", &App::launchOnStartup))
    {
        StartupManager::SetEnabled(App::launchOnStartup);
        ConfigManager::Save();
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Automatically start GammaHotkey when Windows starts");
    }
    ImGui::PopStyleVar();
}

// @TODO: These three sliders are mostly duplicate code. Consolidate?
void RenderBrightnessSlider(Profile& profile, const bool advancedMode)
{
    ImGui::Text("Brightness: %d", profile.brightness);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::SliderInt("##Brightness", &profile.brightness, -50, 50))
    {
        App::state.SetGammaEnabled(true);
        GammaManager::ApplyProfile(profile, App::selectedDisplayIndex);
        if (!advancedMode) ConfigManager::Save();  // Autosave in simple mode.
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Adjust screen brightness (-50 to 50)");
    }
    
    if (advancedMode && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
    {
        if (App::HasSelectedProfile())
        {
            profile.brightness = App::profiles[App::selectedProfileIndex].brightness;
            GammaManager::ApplyProfile(profile, App::selectedDisplayIndex);
        }
    }
    else if (!advancedMode && ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0))
    {
        profile.brightness = 0;
        App::state.SetGammaEnabled(true);
        GammaManager::ApplyProfile(profile, App::selectedDisplayIndex);
        ConfigManager::Save();
    }
}

void RenderContrastSlider(Profile& profile, const bool advancedMode)
{
    ImGui::Text("Contrast: %.3f", profile.contrast);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::SliderFloat("##Contrast", &profile.contrast, 0.5f, 1.5f, "%.3f"))
    {
        App::state.SetGammaEnabled(true);
        GammaManager::ApplyProfile(profile, App::selectedDisplayIndex);
        if (!advancedMode) ConfigManager::Save();  // Autosave in simple mode.
    }
    
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Adjust screen contrast (0.5 to 1.5)");
    }
    
    if (advancedMode && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
    {
        if (App::HasSelectedProfile())
        {
            profile.contrast = App::profiles[App::selectedProfileIndex].contrast;
            App::state.SetGammaEnabled(true);
            GammaManager::ApplyProfile(profile, App::selectedDisplayIndex);
        }
    }
    else if (!advancedMode && ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0))
    {
        profile.contrast = 1.0f;
        App::state.SetGammaEnabled(true);
        GammaManager::ApplyProfile(profile, App::selectedDisplayIndex);
        ConfigManager::Save();
    }
}

void RenderGammaSlider(Profile& profile, const bool advancedMode)
{
    ImGui::Text("Gamma: %.3f", profile.gamma);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::SliderFloat("##Gamma", &profile.gamma, 0.1f, 3.0f, "%.3f"))
    {
        App::state.SetGammaEnabled(true);
        GammaManager::ApplyProfile(profile, App::selectedDisplayIndex);
        if (!advancedMode) ConfigManager::Save();  // Autosave in simple mode.
    }
    
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Adjust gamma curve (0.1 to 3.0)");
    }
    
    if (advancedMode && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
    {
        if (App::HasSelectedProfile())
        {
            profile.gamma = App::profiles[App::selectedProfileIndex].gamma;
            App::state.SetGammaEnabled(true);
            GammaManager::ApplyProfile(profile, App::selectedDisplayIndex);
        }
    }
    else if (!advancedMode && ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0))
    {
        profile.gamma = 1.0f;
        App::state.SetGammaEnabled(true);
        GammaManager::ApplyProfile(profile, App::selectedDisplayIndex);
        ConfigManager::Save();
    }
}

void RenderModeToggleButton()
{
    const ImGuiIO& io = ImGui::GetIO();
    const float buttonWidth = 90.0f;
    const float buttonHeight = 28.0f;
    const float padding = 2.0f;

    // Position in top right, just below title bar.
    const ImVec2 buttonPos = ImVec2(io.DisplaySize.x - buttonWidth - 8.0f, UIConstants::TITLEBAR_HEIGHT + padding);

    // Create an invisible window for the button (required for ImGui widgets to work).
    ImGui::SetNextWindowPos(ImVec2(buttonPos.x - 4.0f, buttonPos.y - 2.0f));
    ImGui::SetNextWindowSize(ImVec2(buttonWidth + 8.0f, buttonHeight + 4.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.06f, 0.07f, 1.0f)); // Dark background.
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.1f, 0.1f, 0.11f, 1.0f)); // Subtle border.

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings;

    if (ImGui::Begin("##ModeToggle", nullptr, flags))
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f)); // Ensure text is visible.

        const char* buttonText = App::state.IsAdvancedModeEnabled() ? "Simple" : "Advanced";
        if (ImGui::Button(buttonText, ImVec2(buttonWidth, buttonHeight)))
        {
            UI::state.targetAdvancedMode = !App::state.IsAdvancedModeEnabled();
            UI::state.modeJustChanged = true;
        }

        ImGui::PopStyleColor(4);

        if (ImGui::IsItemHovered())
        {
            const char* tooltip = App::state.IsAdvancedModeEnabled() ?
                "Switch to simple mode with basic controls only" :
                "Switch to advanced mode with profiles, additional hotkeys, and more options";
            ImGui::SetTooltip("%s", tooltip);
        }
    }
    ImGui::End();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

void RenderTitleBar()
{
    const ImGuiIO& io = ImGui::GetIO();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 windowPos = ImGui::GetWindowPos();

    // Title bar background.
    const ImVec2 titleBarMin = windowPos;
    const ImVec2 titleBarMax = ImVec2(windowPos.x + io.DisplaySize.x, windowPos.y + UIConstants::TITLEBAR_HEIGHT);
    drawList->AddRectFilled(titleBarMin, titleBarMax, IM_COL32(10, 11, 12, 255));

    // About button (left of window controls).
    const float buttonWidth = 46.0f;
    const float aboutButtonWidth = 60.0f;
    const float buttonX = titleBarMax.x - (buttonWidth * 3) - aboutButtonWidth - 8;

    ImGui::SetCursorScreenPos(ImVec2(buttonX, titleBarMin.y));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));

    if (ImGui::Button("About##titlebar", ImVec2(aboutButtonWidth, UIConstants::TITLEBAR_HEIGHT)))
    {
        UI::state.showAboutDialog = true;
    }

    ImGui::SetCursorScreenPos(ImVec2(buttonX + aboutButtonWidth + 8, titleBarMin.y));

    // Minimize button.
    if (ImGui::Button("-##min", ImVec2(buttonWidth, UIConstants::TITLEBAR_HEIGHT)))
    {
        ShowWindow(App::mainWindow, SW_MINIMIZE);
    }
    ImGui::SameLine(0, 0);

    // Maximize/Restore button.
    WINDOWPLACEMENT wp = { sizeof(wp) };
    GetWindowPlacement(App::mainWindow, &wp);
    const bool isMaximized = (wp.showCmd == SW_MAXIMIZE);

    if (ImGui::Button(isMaximized ? "[]##max" : "[]##max", ImVec2(buttonWidth, UIConstants::TITLEBAR_HEIGHT)))
    {
        ShowWindow(App::mainWindow, isMaximized ? SW_RESTORE : SW_MAXIMIZE);
    }
    ImGui::SameLine(0, 0);

    // Close button.
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
    if (ImGui::Button("X##close", ImVec2(buttonWidth, UIConstants::TITLEBAR_HEIGHT)))
    {
        PostMessage(App::mainWindow, WM_CLOSE, 0, 0);
    }
    ImGui::PopStyleColor(4);

    // Title text (left side of title bar) with On/Off indicator.
    ImGui::SetCursorScreenPos(ImVec2(titleBarMin.x + 10, titleBarMin.y + 8));
    const std::wstring statusTextWide = App::GetStatusText();
    const std::string statusText = StringUtils::WideToUTF8(statusTextWide);
    ImGui::Text("%s", statusText.c_str());

    // Make title bar draggable.
    // We use cursor position and offset to manage the drag.
    // @TODO:   Is this the proper way to do this? I tried using a delta position update with mixed results.
    //          This seems to work fine but uncertain if this is best practice.
    const float draggableWidth = buttonX - titleBarMin.x;
    ImGui::SetCursorScreenPos(titleBarMin);
    ImGui::InvisibleButton("##titleDrag", ImVec2(draggableWidth, UIConstants::TITLEBAR_HEIGHT));

    static bool dragging = false;
    static POINT dragOffset = { 0, 0 };

    if (ImGui::IsItemActive())
    {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            // Just started dragging, record offset.
            RECT rect;
            GetWindowRect(App::mainWindow, &rect);
            POINT cursor;
            GetCursorPos(&cursor);
            dragOffset.x = cursor.x - rect.left;
            dragOffset.y = cursor.y - rect.top;
            dragging = true;
        }

        if (dragging)
        {
            // Update window position to follow cursor.
            POINT cursor;
            GetCursorPos(&cursor);
            SetWindowPos(App::mainWindow, nullptr,
                cursor.x - dragOffset.x,
                cursor.y - dragOffset.y,
                0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
    else
    {
        dragging = false;
    }
}

void ClearConflictingHotkey(const UINT vk)
{
    if (App::toggleHotkey == vk) App::toggleHotkey = 0;
    if (App::previousProfileHotkey == vk) App::previousProfileHotkey = 0;
    if (App::nextProfileHotkey == vk) App::nextProfileHotkey = 0;
    
    for (size_t i = 0; i < App::profiles.size(); ++i)
    {
        if (App::profiles[i].hotkey == vk)
        {
            App::profiles[i].hotkey = 0;
        }
    }
}

void ApplyImGuiStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Dark color theme.
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.16f, 0.17f, 0.18f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.16f, 0.17f, 0.18f, 0.95f);
    colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.28f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.21f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.26f, 0.27f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.31f, 0.32f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.11f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.16f, 0.17f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.11f, 0.12f, 0.75f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.11f, 0.12f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.16f, 0.17f, 0.18f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.31f, 0.32f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.41f, 0.42f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.50f, 0.51f, 0.52f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.36f, 0.69f, 1.00f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.36f, 0.69f, 1.00f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.16f, 0.49f, 0.88f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.60f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.25f, 0.28f, 0.50f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.20f, 0.21f, 0.22f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.16f, 0.17f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.21f, 0.22f, 1.00f);

    // Spacing and rounding.
    style.WindowRounding = 0.0f; // No rounding, the Windows window will round if desired.
    style.ChildRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.WindowPadding = ImVec2(12, 12);
    style.FramePadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(8, 6);
    style.ScrollbarSize = 14.0f;
    style.WindowBorderSize = 0.0f; // No border.
}

void DrawGammaCurve()
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    const ImVec2 canvasSize = ImVec2(256, 150);
    
    // Background.
    drawList->AddRectFilled(canvasPos,
        ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
        IM_COL32(250, 250, 250, 255));
    
    // Border.
    drawList->AddRect(canvasPos,
        ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
        IM_COL32(200, 200, 200, 255));
    
    // Grid lines.
    for (int i = 1; i < 4; ++i)
    {
        const float y = canvasPos.y + (i * canvasSize.y / 4);
        drawList->AddLine(ImVec2(canvasPos.x, y),
            ImVec2(canvasPos.x + canvasSize.x, y),
            IM_COL32(220, 220, 220, 255));
    }
    
    // Draw curve.
    const ImU32 curveColor = App::gammaRampFailed ? IM_COL32(220, 53, 69, 255) : IM_COL32(13, 110, 253, 255);
    
    for (int i = 0; i < 255; ++i)
    {
        const float x0 = canvasPos.x + i;
        const float y0 = canvasPos.y + canvasSize.y - (App::lastRamp[i] * canvasSize.y);
        const float x1 = canvasPos.x + i + 1;
        const float y1 = canvasPos.y + canvasSize.y - (App::lastRamp[i + 1] * canvasSize.y);
        
        drawList->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), curveColor, 2.0f);
    }
    
    ImGui::Dummy(canvasSize);
}

void SyncUIWithCurrentProfile()
{
    // Update profile name and hotkey fields.
    if (App::HasSelectedProfile())
    {
        strncpy_s(UI::state.profileNameBuffer, sizeof(UI::state.profileNameBuffer),
            StringUtils::WideToUTF8(App::workingProfile.name).c_str(), _TRUNCATE);
        
        if (App::workingProfile.hotkey != 0)
        {
            strncpy_s(UI::state.profileHotkeyBuffer, sizeof(UI::state.profileHotkeyBuffer),
                StringUtils::WideToUTF8(StringUtils::VkToName(App::workingProfile.hotkey)).c_str(), _TRUNCATE);
        }
        else
        {
            UI::state.profileHotkeyBuffer[0] = '\0';
        }
    }
    else
    {
        UI::state.profileNameBuffer[0] = '\0';
        UI::state.profileHotkeyBuffer[0] = '\0';
    }
}

void OnHotkeyCapture(const UINT vk)
{
    if (UI::state.capturingHotkeyType != -1)
    {
        ApplyHotkeyChange(vk);
    }
}
