// Copyright (c) 2025 Max Godman

// Coordinates rendering the UI, for both Simple and Advanced UI modes.

#include "AppGlobals.h"
#include "UIGlobals.h"
#include "ConfigManager.h"

void RenderSimpleUI();
void RenderAdvancedUI();
void RenderAllDialogs();

/**
 * Main UI coordinator, called every frame in the main message loop.
 */
void RenderMainUI()
{
    // Handle mode change with proper window resize.
    if (UI::state.modeJustChanged)
    {
        // @TODO:   Refactor this. Ended up with this when battling unknown issues using imgui and switching between simple/advanced.
        //          UI rendering should be decoupled from this, but need to investigate how to handle the switch properly.
        App::state.SetAdvancedModeEnabled(UI::state.targetAdvancedMode);
        ConfigManager::Save();
        
        UI::state.modeJustChanged = false;

        App::SyncWindowSizeToState();
        
        // Skip popup rendering this frame, ImGui state is in transition.
        if (App::state.IsAdvancedModeEnabled())
            RenderAdvancedUI();
        else
            RenderSimpleUI();
        return;  // Don't render popups this frame.
    }
    
    // Render appropriate UI (this creates the ImGui window context).
    if (App::state.IsAdvancedModeEnabled())
        RenderAdvancedUI();
    else
        RenderSimpleUI();
    
    // Render all dialogs after main window is created.
    RenderAllDialogs();
}
