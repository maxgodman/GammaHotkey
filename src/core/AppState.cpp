// Copyright (c) 2025 Max Godman

#include "AppState.h"

// Forward declaration, implemented in UIGlobals.cpp. Declared here (rather than including
// the header) to keep the core state object loosely coupled to the UI layer.
namespace UI { void SyncUIToState(); }

void AppState::SetConfigInitialized(const bool initialized)
{
	m_configInitialized = initialized;
}

void AppState::SetGammaEnabled(const bool enabled)
{
	m_gammaEnabled = enabled;

	// Refresh the tray icon/tooltip so it always reflects the on/off state, no matter
	// which code path toggled it. Safe to call before the tray icon exists (it no-ops).
	UI::SyncUIToState();
}

void AppState::SetAdvancedModeEnabled(const bool enabled)
{
	m_advancedModeEnabled = enabled;
}
