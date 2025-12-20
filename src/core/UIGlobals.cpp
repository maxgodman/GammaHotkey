// Copyright (c) 2025 Max Godman

#include "UIGlobals.h"
#include "AppGlobals.h"
#include "IconManager.h"

namespace UI
{
    UIState state;

    void SyncUIToState()
    {
        IconManager::UpdateAllIcons(App::state.IsGammaEnabled());
    }
}
