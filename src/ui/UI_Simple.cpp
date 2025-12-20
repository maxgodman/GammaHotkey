// Copyright (c) 2025 Max Godman

// Simple mode UI.

#include "framework.h"
#include "imgui.h"
#include "AppGlobals.h"
#include "UIGlobals.h"
#include "UI_Shared.h"
#include "StringUtils.h"

void RenderSimpleUI()
{
    const ImGuiIO& io = ImGui::GetIO();
    
    // Vertical panel window.
    const ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                     ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    
    if (!ImGui::Begin("GammaHotkey##Simple", nullptr, window_flags))
    {
        ImGui::PopStyleVar(3);
        ImGui::End();
        return;
    }
    
    ImGui::PopStyleVar(3);
    
    // Title bar.
    RenderTitleBar();
    
    // @TODO: Rework the below, not working as desired.
    // Content area starts directly after title bar, with padding handled by child.
    ImGui::SetCursorPosY(UIConstants::TITLEBAR_HEIGHT);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 12));

    // Use remaining height for content area.
    const float contentHeight = io.DisplaySize.y - UIConstants::TITLEBAR_HEIGHT;
    ImGui::BeginChild("SimpleContent", ImVec2(0, contentHeight), false);
    
    // Add manual spacing since borderless child doesn't auto-apply WindowPadding?
    ImGui::Dummy(ImVec2(0, 0));  // Force padding to apply.
    
    // Display selector.
    RenderDisplayComboBox();
    
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Sliders using reusable components.
    RenderBrightnessSlider(App::simpleProfile, false);
    ImGui::Spacing();
    RenderContrastSlider(App::simpleProfile, false);
    ImGui::Spacing();
    RenderGammaSlider(App::simpleProfile, false);
    
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Toggle hotkey.
    ImGui::Text("Toggle On/Off Hotkey");
    ImGui::SetNextItemWidth(-120);
    std::string toggleKeyName = StringUtils::WideToUTF8(StringUtils::VkToName(App::toggleHotkey));
    ImGui::InputText("##ToggleHotkey", (char*)toggleKeyName.c_str(), toggleKeyName.size() + 1, ImGuiInputTextFlags_ReadOnly);
    ImGui::SameLine();
    if (ImGui::Button("Set", ImVec2(100, 0)))
    {
        UI::state.showHotkeyCapture = true;
        UI::state.capturingHotkeyType = 0;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Set a hotkey to toggle gamma adjustments on/off");
    }
    
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Collapsible options.
    if (ImGui::CollapsingHeader("Options", ImGuiTreeNodeFlags_None))
    {
        RenderOptionsCheckboxes();
    }
    
    ImGui::EndChild(); // SimpleContent.
    ImGui::PopStyleVar();
    
    // Render mode toggle button (overlaid in top right).
    RenderModeToggleButton();
    
    ImGui::End();
}
