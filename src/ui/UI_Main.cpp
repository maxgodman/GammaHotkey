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
    // The mode toggle button (RenderModeToggleButton) never switches modes inline: clicking it only
    // records the request (targetAdvancedMode) and sets modeJustChanged. The switch is applied here,
    // at the very top of the next frame's UI build - after NewFrame() but before any ImGui window is
    // begun. Deferring it to this point is deliberate and load-bearing:
    //
    //   - Switching modes changes the window (and swap-chain) size. SyncWindowSizeToState() calls
    //     SetWindowPos, which delivers WM_SIZE synchronously, and WM_SIZE resizes the DX11 swap-chain
    //     buffers (ImGuiRenderer::OnResize). Running that from the button handler would recreate the
    //     render target deep inside the previous mode's window hierarchy, mid-frame.
    //   - Running it here instead, the resize happens while no ImGui window is open and no draw data
    //     has been emitted, so recreating the render target is safe and exactly one mode's layout (the
    //     new one) is built for the frame. WM_SIZE must not render (a nested NewFrame() would assert);
    //     it only resizes the buffers - see the WM_SIZE handler in main.cpp.
    if (UI::state.modeJustChanged)
    {
        App::state.SetAdvancedModeEnabled(UI::state.targetAdvancedMode);
        ConfigManager::Save();
        
        UI::state.modeJustChanged = false;

        App::SyncWindowSizeToState();
        
        // Render the new mode's UI this frame so the swap chain is not presented blank, but skip the
        // dialogs: NewFrame() captured io.DisplaySize before the resize above, so it is momentarily
        // stale and popups auto-center off it. The next frame renders everything at the settled size;
        // a pending dialog request is a latched one-shot flag (see UIState.h), so nothing is lost.
        if (App::state.IsAdvancedModeEnabled())
            RenderAdvancedUI();
        else
            RenderSimpleUI();
        return;
    }
    
    // Render appropriate UI (this creates the ImGui window context).
    if (App::state.IsAdvancedModeEnabled())
        RenderAdvancedUI();
    else
        RenderSimpleUI();
    
    // Render all dialogs after main window is created.
    RenderAllDialogs();
}
