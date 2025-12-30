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

    // Content area.
    ImGui::SetCursorPosY(UIConstants::TITLEBAR_HEIGHT);

    const float contentHeight = io.DisplaySize.y - UIConstants::TITLEBAR_HEIGHT;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(UIConstants::CONTENT_PADDING_X, UIConstants::CONTENT_PADDING_Y));

    if (ImGui::BeginChild("SimpleContent", ImVec2(0, contentHeight), ImGuiChildFlags_AlwaysUseWindowPadding))
    {
        RenderDisplayComboBox();

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

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

        const std::string toggleKeyName = StringUtils::WideToUTF8(StringUtils::VkToName(App::toggleHotkey));

        const float buttonWidth = 50.0f;
        const float spacing = ImGui::GetStyle().ItemSpacing.x;

        // Display as text in a frame.
        ImGui::BeginDisabled();
        char buf[64];
        strncpy_s(buf, toggleKeyName.c_str(), sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - buttonWidth - spacing);
        ImGui::InputText("##ToggleHotkey", buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);
        ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Set", ImVec2(buttonWidth, 0)))
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

        if (ImGui::CollapsingHeader("Options", ImGuiTreeNodeFlags_None))
        {
            RenderOptionsCheckboxes();
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(); // WindowPadding.

    RenderModeToggleButton();

    ImGui::End();
}
