// Copyright (c) 2025 Max Godman

#include "AppState.h"

void AppState::SetConfigInitialized(const bool initialized)
{
	m_configInitialized = initialized;
}

void AppState::SetGammaEnabled(const bool enabled)
{
	m_gammaEnabled = enabled;
}

void AppState::SetAdvancedModeEnabled(const bool enabled)
{
	m_advancedModeEnabled = enabled;
}
