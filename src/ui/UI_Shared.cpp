// Copyright (c) 2025 Max Godman

// Shared UI components and utilities.

#include "framework.h"
#include "UI_Shared.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "AppGlobals.h"
#include "UIGlobals.h"
#include "GammaManager.h"
#include "ConfigManager.h"
#include "StartupManager.h"
#include "HotkeyManager.h"
#include "StringUtils.h"
#include <string>
#include <type_traits>
#include <cstdio>

/**
 * @brief Check if the specified window is maximized.
 * @return true = window is maximized.
 */
static bool IsWindowMaximized(HWND hwnd)
{
    WINDOWPLACEMENT wp = { sizeof(wp) };
    GetWindowPlacement(hwnd, &wp);
    return wp.showCmd == SW_MAXIMIZE;
}

// The three custom-drawn window-control buttons. Maximize doubles as Restore when the window is
// maximized. Clicking is handled natively in the WndProc (these are non-client caption buttons);
// this file only draws them and publishes their rects. See RenderTitleBar and WM_NCLBUTTONUP.
enum class CaptionButton { Minimize, MaximizeRestore, Close };

/**
 * @brief Draws one native-style caption button into a screen-space rect: a hover background plus
 *        the glyph as vector primitives, which stay crisp at any DPI (unlike the stretched bitmap
 *        UI font) and need no icon-font dependency.
 * @param hovered Whether the OS cursor is over this button. Computed from the cursor position in
 *        RenderTitleBar rather than from ImGui, because these buttons are non-client and ImGui
 *        never sees the mouse over them.
 * @param maximized Selects the Restore glyph (two offset squares) over the Maximize glyph.
 * @param dpiScale Factor for the glyph metrics below. Passed in rather than queried here so the
 *        whole title bar is drawn from the one value RenderTitleBar sampled for this frame.
 */
static void DrawCaptionButton(ImDrawList* dl, const CaptionButton kind, const ImVec2& bmin,
    const ImVec2& bmax, const bool hovered, const bool maximized, const float dpiScale)
{
    if (hovered)
    {
        const ImU32 bg = (kind == CaptionButton::Close)
            ? IM_COL32(0xC4, 0x2B, 0x1C, 0xFF)  // Windows 11 close red.
            : IM_COL32(60, 60, 60, 255);        // Subtle gray for minimize/maximize.
        dl->AddRectFilled(bmin, bmax, bg);
    }

    const ImVec2 c((bmin.x + bmax.x) * 0.5f, (bmin.y + bmax.y) * 0.5f);
    const ImU32 fg = IM_COL32(230, 230, 230, 255);
    const float t = 1.0f * dpiScale;  // Glyph stroke thickness.
    const float r = 5.0f * dpiScale;  // Glyph half-extent.
    const float inset = 2.0f * dpiScale;  // Offset between the Restore glyph's two squares.

    switch (kind)
    {
    case CaptionButton::Minimize:
        dl->AddLine(ImVec2(c.x - r, c.y), ImVec2(c.x + r, c.y), fg, t);
        break;
    case CaptionButton::MaximizeRestore:
        if (maximized)
        {
            // Restore: a back square peeking out up-and-right behind a front square.
            dl->AddRect(ImVec2(c.x - r + inset, c.y - r), ImVec2(c.x + r, c.y + r - inset), fg, 0.0f, 0, t);
            dl->AddRect(ImVec2(c.x - r, c.y - r + inset), ImVec2(c.x + r - inset, c.y + r), fg, 0.0f, 0, t);
        }
        else
        {
            dl->AddRect(ImVec2(c.x - r, c.y - r), ImVec2(c.x + r, c.y + r), fg, 0.0f, 0, t);
        }
        break;
    case CaptionButton::Close:
        dl->AddLine(ImVec2(c.x - r, c.y - r), ImVec2(c.x + r, c.y + r), fg, t);
        dl->AddLine(ImVec2(c.x - r, c.y + r), ImVec2(c.x + r, c.y - r), fg, t);
        break;
    }
}

/**
 * @brief Check whether a key is already bound to an action other than the one being captured.
 * @param vk Virtual key to check.
 * @param captureTarget The action currently being (re)bound; its own existing binding is ignored.
 * @return Friendly name of the conflicting action, or empty if there is no conflict.
 *
 * Skipping @p captureTarget is what lets a hotkey be rebound to the key it already has without
 * being reported as conflicting with itself: the action's current binding is not a conflict for
 * that same action, only for a different one.
 */
static std::string CheckHotkeyConflict(const UINT vk, const HotkeyCapture captureTarget)
{
    if (vk == 0) return "";

    if (captureTarget != HotkeyCapture::TOGGLE && App::toggleHotkey == vk)
        return "Toggle On/Off";
    if (captureTarget != HotkeyCapture::PREVIOUS_PROFILE && App::previousProfileHotkey == vk)
        return "Previous Profile";
    if (captureTarget != HotkeyCapture::NEXT_PROFILE && App::nextProfileHotkey == vk)
        return "Next Profile";

    for (size_t i = 0; i < App::profiles.size(); ++i)
    {
        // When rebinding an existing profile's hotkey, that same profile's current binding
        // is not a conflict. A brand-new profile (selectedProfileIndex out of range) isn't in
        // this array yet, so nothing is skipped and every profile is checked.
        if (captureTarget == HotkeyCapture::PROFILE && (int)i == App::selectedProfileIndex)
            continue;

        if (App::profiles[i].hotkey == vk)
        {
            return "Profile: " + StringUtils::WideToUTF8(App::profiles[i].name);
        }
    }

    return ""; // No conflict.
}

void SetHotkeyForCaptureTarget(const UINT vk)
{
    switch (UI::state.capturingHotkeyType)
    {
    case HotkeyCapture::TOGGLE:
        App::toggleHotkey = vk;
        break;
    case HotkeyCapture::PREVIOUS_PROFILE:
        App::previousProfileHotkey = vk;
        break;
    case HotkeyCapture::NEXT_PROFILE:
        App::nextProfileHotkey = vk;
        break;
    case HotkeyCapture::PROFILE:
        // An existing profile is edited in place in the profiles array; a profile that
        // hasn't been saved yet lives only in workingProfile. Always update workingProfile
        // so the pending edit survives a "Save New Profile".
        if (App::selectedProfileIndex >= 0 && App::selectedProfileIndex < (int)App::profiles.size())
            App::profiles[App::selectedProfileIndex].hotkey = vk;
        App::workingProfile.hotkey = vk;

        // Keep the display buffer in sync (empty when the binding is cleared).
        if (vk == 0)
            UI::state.profileHotkeyBuffer[0] = '\0';
        else
            strncpy_s(UI::state.profileHotkeyBuffer, sizeof(UI::state.profileHotkeyBuffer),
                      StringUtils::WideToUTF8(StringUtils::VkToName(vk)).c_str(), _TRUNCATE);
        break;
    case HotkeyCapture::NONE:
        break;
    }
}

/**
 * @brief Apply hotkey change after capture.
 */
static void ApplyHotkeyChange(const UINT vk)
{
    // Check for conflicts with a *different* action. Rebinding the action being captured
    // to the key it already has is not a conflict, so it applies silently below.
    const std::string conflict = CheckHotkeyConflict(vk, UI::state.capturingHotkeyType);
    if (!conflict.empty())
    {
        UI::state.showHotkeyConflict = true;
        UI::state.conflictingHotkey = vk;
        UI::state.conflictDescription = conflict;
        return;
    }
    
    // Apply the new hotkey (or clear it if vk is 0).
    SetHotkeyForCaptureTarget(vk);

    // Always save when assigning hotkeys.
    ConfigManager::Save();
    
    // Mark that we're done capturing, this will trigger hotkey re-registration.
    UI::state.capturingHotkeyType = HotkeyCapture::NONE;
    UI::state.closeCapturePopup = true;  // Request popup closure.
}

void RenderDisplayComboBox()
{
    ImGui::Text("Display");
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    // Determine the preview text for the combo box.
    const char* previewText = "No displays";
    std::string previewStorage;
    if (!App::displays.empty())
    {
        if (App::selectedDisplayIndex == -1)
            previewText = "All displays";
        else
        {
            previewStorage = StringUtils::WideToUTF8(App::displays[App::selectedDisplayIndex].friendlyName);
            previewText = previewStorage.c_str();
        }
    }

    if (ImGui::BeginCombo("##Display", previewText))
    {
        // All displays option.
        {
            const bool selected = (App::selectedDisplayIndex == -1);
            if (ImGui::Selectable("All displays##all", selected))
            {
                if (App::selectedDisplayIndex != -1)
                {
                    // Reset gamma on the previous single display before switching.
                    GammaManager::ResetDisplay(App::selectedDisplayIndex);
                }
                App::selectedDisplayIndex = -1;
                App::SyncGammaToState();
                ConfigManager::Save();
            }
            if (selected)
                ImGui::SetItemDefaultFocus();
        }

        // Individual display options.
        for (int i = 0; i < (int)App::displays.size(); ++i)
        {
            const bool selected = (App::selectedDisplayIndex == i);

            // Create label with unique ID: "Display Name##index"
            std::string label = StringUtils::WideToUTF8(App::displays[i].friendlyName) + "##" + std::to_string(i);

            if (ImGui::Selectable(label.c_str(), selected))
            {
                if (App::selectedDisplayIndex != i)
                {
                    // Reset gamma on the previous display before switching.
                    GammaManager::ResetDisplay(App::selectedDisplayIndex);
                }
                App::selectedDisplayIndex = i;
                App::SyncGammaToState();
                ConfigManager::Save();
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
    // Only the x needs the factor: the y is read back from the style, which ScaleAllSizes() has
    // already scaled, while CHECKBOX_INNERSPACING is our own literal.
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing,
        ImVec2(UIConstants::CHECKBOX_INNERSPACING * App::GetDpiScale(), ImGui::GetStyle().ItemInnerSpacing.y));
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
        std::wstring detail;
        if (!StartupManager::SetEnabled(App::launchOnStartup, &detail))
        {
            // The shortcut wasn't written/removed, so the checkbox is lying. Reflect the real
            // on-disk state, then surface the failure instead of silently swallowing it.
            App::launchOnStartup = StartupManager::IsEnabled();
            UI::state.startupShortcutErrorDetail = StringUtils::WideToUTF8(detail);
            UI::state.showStartupShortcutError = true;
        }
        ConfigManager::Save();
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Automatically start GammaHotkey when Windows starts");
    }
    ImGui::PopStyleVar();
}

// The brightness/contrast/gamma sliders differ only in label, value type (int vs float),
// range, simple-mode reset target, and tooltip; the rest (apply-on-change, deferred autosave,
// advanced-mode double-click restore-from-saved, simple-mode double-click reset-to-default) is
// identical. This shared helper carries all of it; the three public functions are thin wrappers.
//
// @p member selects the Profile field so the same pointer is reused to read/write @p profile and
// to read the saved profile for the advanced-mode restore, keeping every id and code path exactly
// as the three hand-written versions were.
template <typename T>
static void RenderProfileSlider(Profile& profile, const bool advancedMode, const char* label,
                                T Profile::* member, const T minValue, const T maxValue,
                                const T defaultValue, const char* tooltip)
{
    T& value = profile.*member;
    const std::string sliderId = std::string("##") + label;

    bool changed;
    if constexpr (std::is_integral_v<T>)
    {
        ImGui::Text("%s: %d", label, value);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        changed = ImGui::SliderInt(sliderId.c_str(), &value, minValue, maxValue);
    }
    else
    {
        ImGui::Text("%s: %.3f", label, value);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        changed = ImGui::SliderFloat(sliderId.c_str(), &value, minValue, maxValue, "%.3f");
    }

    if (changed)
    {
        App::state.SetGammaEnabled(true);
        GammaManager::ApplyProfile(profile, App::selectedDisplayIndex);
    }
    // Autosave in simple mode, but only once the drag/edit finishes (not every frame).
    if (!advancedMode && ImGui::IsItemDeactivatedAfterEdit())
    {
        ConfigManager::Save();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("%s", tooltip);
    }

    if (advancedMode && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
    {
        if (App::HasSelectedProfile())
        {
            value = App::profiles[App::selectedProfileIndex].*member;
            App::state.SetGammaEnabled(true);
            GammaManager::ApplyProfile(profile, App::selectedDisplayIndex);
        }
    }
    else if (!advancedMode && ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0))
    {
        value = defaultValue;
        App::state.SetGammaEnabled(true);
        GammaManager::ApplyProfile(profile, App::selectedDisplayIndex);
        ConfigManager::Save();
    }
}

// Build a slider's tooltip by appending its permitted range to the description, so the
// displayed bounds always track ProfileRange instead of a duplicated literal. Each caller
// stashes the result in a function-local static, so this runs once, not every frame.
static std::string MakeSliderTooltip(const char* description, int minValue, int maxValue)
{
    return std::string(description) + " (" + std::to_string(minValue) + " to " + std::to_string(maxValue) + ")";
}

static std::string MakeSliderTooltip(const char* description, float minValue, float maxValue)
{
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%s (%.1f to %.1f)", description, minValue, maxValue);
    return buffer;
}

void RenderBrightnessSlider(Profile& profile, const bool advancedMode)
{
    static const std::string tooltip = MakeSliderTooltip("Adjust screen brightness",
                                                         ProfileRange::BRIGHTNESS_MIN, ProfileRange::BRIGHTNESS_MAX);
    RenderProfileSlider<int>(profile, advancedMode, "Brightness", &Profile::brightness,
                             ProfileRange::BRIGHTNESS_MIN, ProfileRange::BRIGHTNESS_MAX,
                             ProfileRange::BRIGHTNESS_DEFAULT, tooltip.c_str());
}

void RenderContrastSlider(Profile& profile, const bool advancedMode)
{
    static const std::string tooltip = MakeSliderTooltip("Adjust screen contrast",
                                                         ProfileRange::CONTRAST_MIN, ProfileRange::CONTRAST_MAX);
    RenderProfileSlider<float>(profile, advancedMode, "Contrast", &Profile::contrast,
                               ProfileRange::CONTRAST_MIN, ProfileRange::CONTRAST_MAX,
                               ProfileRange::CONTRAST_DEFAULT, tooltip.c_str());
}

void RenderGammaSlider(Profile& profile, const bool advancedMode)
{
    static const std::string tooltip = MakeSliderTooltip("Adjust gamma curve",
                                                         ProfileRange::GAMMA_MIN, ProfileRange::GAMMA_MAX);
    RenderProfileSlider<float>(profile, advancedMode, "Gamma", &Profile::gamma,
                               ProfileRange::GAMMA_MIN, ProfileRange::GAMMA_MAX,
                               ProfileRange::GAMMA_DEFAULT, tooltip.c_str());
}

void RenderModeToggleButton()
{
    const ImGuiIO& io = ImGui::GetIO();

    // The toggle lives in its own small borderless window pinned to the top-right corner, just below
    // the custom title bar, so it stays anchored regardless of the content laid out beneath it. The
    // host window is sized to wrap the button exactly (button size plus WindowPadding on every side)
    // and positioned so the button itself lands at buttonPos.
    //
    // Every metric here is one of our own literals rather than an ImGui style value, so none of it
    // is covered by style.ScaleAllSizes() and all of it needs the DPI factor - without it the whole
    // cluster (button, its background window and its outline) stays at 100% size while the label
    // inside grows with the font, and the text clips.
    const float dpiScale = App::GetDpiScale();
    // Sized for the longer of the two labels it alternates between, so the button does not change
    // width when the mode is switched.
    const float buttonWidth = ImMax(GetScaledButtonWidth("Advanced", 90.0f),
                                    GetScaledButtonWidth("Simple", 90.0f));
    const float buttonHeight = 28.0f * dpiScale;
    const float rightMargin = 8.0f * dpiScale;         // Gap from the parent window's right edge.
    const float topMargin = 2.0f * dpiScale;           // Gap below the title bar.
    const ImVec2 windowPadding = ImVec2(4.0f * dpiScale, 2.0f * dpiScale);

    const ImVec2 buttonPos = ImVec2(io.DisplaySize.x - buttonWidth - rightMargin,
                                    GetTitleBarHeight() + topMargin);

    ImGui::SetNextWindowPos(ImVec2(buttonPos.x - windowPadding.x, buttonPos.y - windowPadding.y));
    ImGui::SetNextWindowSize(ImVec2(buttonWidth + windowPadding.x * 2.0f, buttonHeight + windowPadding.y * 2.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, windowPadding);
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

        // The label shows the mode this button switches TO. The switch is intentionally deferred:
        // record the target and flag it, then RenderMainUI applies it at the top of the next frame
        // (see UI_Main.cpp), where resizing the window/swap chain is safe.
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

float GetScaledButtonWidth(const char* label, const float nominalWidth)
{
    return ImMax(nominalWidth * App::GetDpiScale(),
                 ImGui::CalcTextSize(label).x + ImGui::GetStyle().FramePadding.x * 2.0f);
}

float GetTitleBarHeight()
{
    return UIConstants::TITLEBAR_HEIGHT * App::GetDpiScale();
}

void RenderTitleBar()
{
    const ImGuiIO& io = ImGui::GetIO();
    // Sampled once per frame and threaded through everything below, so the drawn geometry and the
    // hit-test rects published at the end cannot be computed from two different factors.
    const float dpiScale = App::GetDpiScale();
    const float titleBarHeight = GetTitleBarHeight();
    // Draw the title bar to the foreground draw list so it renders above the modal dim (which
    // darkens the rest of the window while a dialog is open), keeping the title bar "untouched".
    // Nothing overlaps this top strip, so drawing it foreground is visually identical otherwise.
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    const ImVec2 windowPos = ImGui::GetWindowPos();
    const bool maximized = IsWindowMaximized(App::mainWindow);

    // Title bar background.
    const ImVec2 titleBarMin = windowPos;
    const ImVec2 titleBarMax = ImVec2(windowPos.x + io.DisplaySize.x, windowPos.y + titleBarHeight);
    drawList->AddRectFilled(titleBarMin, titleBarMax, IM_COL32(10, 11, 12, 255));

    // About button (left of the window controls), kept as a normal ImGui button so it stays
    // HTCLIENT and ImGui handles its click.
    const float buttonWidth = 46.0f * dpiScale;
    const char* aboutLabel = "About";
    const ImVec2 aboutTextSize = ImGui::CalcTextSize(aboutLabel);
    const float aboutButtonWidth = GetScaledButtonWidth(aboutLabel, 60.0f);
    const float buttonX = titleBarMax.x - (buttonWidth * 3) - aboutButtonWidth - 8.0f * dpiScale;

    // The interaction is a plain (invisible) ImGui button so hover/press behavior is unchanged;
    // the visual is drawn to the foreground draw list so it stays bright over the modal dim.
    const ImVec2 aboutMin(buttonX, titleBarMin.y);
    const ImVec2 aboutMax(buttonX + aboutButtonWidth, titleBarMin.y + titleBarHeight);
    ImGui::SetCursorScreenPos(aboutMin);
    if (ImGui::InvisibleButton("About##titlebar", ImVec2(aboutButtonWidth, titleBarHeight)))
    {
        UI::state.showAboutDialog = true;
    }
    if (ImGui::IsItemActive())
        drawList->AddRectFilled(aboutMin, aboutMax, IM_COL32(51, 51, 51, 255));
    else if (ImGui::IsItemHovered())
        drawList->AddRectFilled(aboutMin, aboutMax, IM_COL32(77, 77, 77, 255));

    const ImVec2 aboutTextPos(aboutMin.x + (aboutButtonWidth - aboutTextSize.x) * 0.5f,
                              aboutMin.y + (titleBarHeight - aboutTextSize.y) * 0.5f);
    drawList->AddText(aboutTextPos, ImGui::GetColorU32(ImGuiCol_Text), aboutLabel);

    // Window controls: minimize / maximize-restore / close, custom-drawn as non-client caption
    // buttons (WM_NCHITTEST reports HTMINBUTTON/HTMAXBUTTON/HTCLOSE and the WndProc runs the action
    // on click; here we only draw them and publish their rects). Being non-client, ImGui never sees
    // the mouse over them, so hover comes from the OS cursor position; WindowFromPoint suppresses the
    // highlight when another window occludes the button under the cursor.
    POINT screenCursor;
    GetCursorPos(&screenCursor);
    const bool cursorOverWindow = (WindowFromPoint(screenCursor) == App::mainWindow);
    POINT cursor = screenCursor;
    ScreenToClient(App::mainWindow, &cursor);

    const struct { CaptionButton kind; float left; RECT* out; } controls[3] = {
        { CaptionButton::Minimize,        titleBarMax.x - buttonWidth * 3.0f, &UI::state.titleBar.minButton },
        { CaptionButton::MaximizeRestore, titleBarMax.x - buttonWidth * 2.0f, &UI::state.titleBar.maxButton },
        { CaptionButton::Close,           titleBarMax.x - buttonWidth * 1.0f, &UI::state.titleBar.closeButton },
    };

    for (const auto& ctrl : controls)
    {
        const ImVec2 bmin(ctrl.left, titleBarMin.y);
        const ImVec2 bmax(ctrl.left + buttonWidth, titleBarMin.y + titleBarHeight);

        // Client-space rect for the WndProc hit test. windowPos is (0,0) for the main window, but
        // subtract it so this holds regardless of where the ImGui window sits.
        ctrl.out->left   = (LONG)(bmin.x - windowPos.x);
        ctrl.out->top    = (LONG)(bmin.y - windowPos.y);
        ctrl.out->right  = (LONG)(bmax.x - windowPos.x);
        ctrl.out->bottom = (LONG)(bmax.y - windowPos.y);

        const bool hovered = cursorOverWindow &&
            cursor.x >= ctrl.out->left && cursor.x < ctrl.out->right &&
            cursor.y >= ctrl.out->top && cursor.y < ctrl.out->bottom;

        DrawCaptionButton(drawList, ctrl.kind, bmin, bmax, hovered, maximized, dpiScale);
    }

    // On/Off indicator: a small filled circle on the left of the title bar reflecting whether
    // gamma is currently applied. Read straight from state here because ImGui is immediate-mode
    // and this repaints every frame; nothing needs to push updates to it.
    const float indicatorRadius = ImMax(3.0f * dpiScale, titleBarHeight * 0.14f);
    const ImVec2 indicatorCenter = ImVec2(titleBarMin.x + 10.0f * dpiScale + indicatorRadius,
                                          titleBarMin.y + titleBarHeight * 0.5f);
    const ImU32 indicatorColor = App::state.IsGammaEnabled()
        ? IM_COL32(80, 200, 120, 255)   // Green-ish: gamma on.
        : IM_COL32(90, 92, 96, 255);    // Dim gray: gamma off.
    drawList->AddCircleFilled(indicatorCenter, indicatorRadius, indicatorColor);

    // Title text (left side of title bar), positioned to the right of the indicator. Drawn via the
    // (foreground) draw list so it stays bright over the modal dim.
    const std::wstring statusTextWide = App::GetStatusText();
    const std::string statusText = StringUtils::WideToUTF8(statusTextWide);
    // Centered against the bar rather than nudged down by a fixed offset, matching the About
    // label above: the text height grows with both DPI and the UI font, so a literal would drift
    // off-centre at every scale but the one it was tuned at.
    const ImVec2 titleTextPos(indicatorCenter.x + indicatorRadius + 8.0f * dpiScale,
                              titleBarMin.y + (titleBarHeight - ImGui::GetTextLineHeight()) * 0.5f);
    drawList->AddText(titleTextPos, ImGui::GetColorU32(ImGuiCol_Text), statusText.c_str());

    // Publish the draggable region for WM_NCHITTEST to report as HTCAPTION, handing the drag to the
    // OS move loop. That restores Aero Snap, the taskbar peek (the window can no longer be stranded
    // behind the taskbar), window-shake, and double-click/drag-off maximize, so none of those need
    // handling here. The strip runs from the left edge to buttonX; the button cluster to its right
    // stays HTCLIENT so ImGui keeps its clicks. buttonX and windowPos are ImGui screen coordinates;
    // for the main window (pinned at 0,0) those equal the client coordinates WM_NCHITTEST uses.
    UI::state.titleBar.captionBottom = (int)titleBarHeight;
    UI::state.titleBar.dragRight = (int)(buttonX - windowPos.x);
    UI::state.titleBar.valid = true;
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
    colors[ImGuiCol_PopupBg] = ImVec4(0.16f, 0.17f, 0.18f, 1.00f); // Fully opaque so dialogs don't show the UI through them.
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.50f); // Dark fade behind dialogs. The title bar draws to the foreground draw list (RenderTitleBar), so it stays above this.
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

    // Design size of the embedded font (see ImGui_Integration and FONT.md), before the global
    // factors are applied - ImGui multiplies this by style.FontScaleDpi to get the rasterized
    // size. It lives here, with the rest of the style, rather than next to the AddFont call,
    // because ImGuiRenderer::OnDpiChanged resets the whole style (style = ImGuiStyle()) and then
    // calls this function: setting it anywhere else would silently revert the font to ImGui's
    // default base size after the first DPI change.
    style.FontSizeBase = 15.0f;

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

    // Scale the hardcoded pixel metrics below by the window's DPI so the graph keeps the same
    // visual proportions at any scale. GetContentRegionAvail() is already in physical pixels (it
    // reflects the DPI-scaled window and style), so only these literals need the factor. At 100%
    // DPI the factor is 1.0, leaving every size unchanged.
    const float dpiScale = App::GetDpiScale();
    const float minCanvasWidth = 150.0f * dpiScale;
    const float minCanvasHeight = 100.0f * dpiScale;
    const float lineThickness = 1.0f * dpiScale;

    const ImVec2 canvasSize = ImVec2(
        ImMax(ImGui::GetContentRegionAvail().x, minCanvasWidth),
        ImMax(ImGui::GetContentRegionAvail().y, minCanvasHeight)
    );

    // Background.
    drawList->AddRectFilled(canvasPos,
        ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
        IM_COL32(250, 250, 250, 255));

    // Border.
    drawList->AddRect(canvasPos,
        ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
        IM_COL32(200, 200, 200, 255), 0.0f, 0, lineThickness);

    // Grid lines.
    for (int i = 1; i < 4; ++i)
    {
        const float y = canvasPos.y + (i * canvasSize.y / 4);
        drawList->AddLine(ImVec2(canvasPos.x, y),
            ImVec2(canvasPos.x + canvasSize.x, y),
            IM_COL32(220, 220, 220, 255), lineThickness);
    }

    // Draw curve.
    const ImU32 curveColor = App::state.gammaRampFailed ? IM_COL32(220, 53, 69, 255) : IM_COL32(13, 110, 253, 255);
    const float curveThickness = 2.0f * dpiScale;

    for (int i = 0; i < 255; ++i)
    {
        const float x0 = canvasPos.x + (i / 255.0f) * canvasSize.x;
        const float y0 = canvasPos.y + canvasSize.y - (App::state.lastRamp[i] * canvasSize.y);
        const float x1 = canvasPos.x + ((i + 1) / 255.0f) * canvasSize.x;
        const float y1 = canvasPos.y + canvasSize.y - (App::state.lastRamp[i + 1] * canvasSize.y);
        
        drawList->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), curveColor, curveThickness);
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
    if (UI::state.capturingHotkeyType == HotkeyCapture::NONE)
        return; // Not capturing.

    // Reject keys that can't be used as hotkeys (bare modifiers, F12).
    // Keep the capture dialog open and show the user why, so they can press another key.
    const char* reason = nullptr;
    if (!HotkeyManager::IsBindableKey(vk, &reason))
    {
        UI::state.captureRejectReason = reason ? reason : "That key can't be used as a hotkey.";
        return;
    }

    UI::state.captureRejectReason.clear();
    ApplyHotkeyChange(vk);
}
