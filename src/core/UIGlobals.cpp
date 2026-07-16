// Copyright (c) 2025 Max Godman

#include "UIGlobals.h"
#include "AppGlobals.h"
#include "SystemTrayManager.h"

namespace UI
{
    UIState state;

    void SyncUIToState()
    {
        // Only the tray icon tracks gamma state. The window/taskbar icon stays fixed; the on/off
        // indicator lives in the custom title bar, which reads state directly each frame - see
        // RenderTitleBar.
        SystemTrayManager::UpdateIcon(App::state.IsGammaEnabled());
    }
}
