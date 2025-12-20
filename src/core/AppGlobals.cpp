// Copyright (c) 2025 Max Godman

#include "AppGlobals.h"
#include "GammaManager.h"

namespace App
{
    AppState state;

    HWND mainWindow = nullptr;

    std::vector<DisplayEntry> displays;
    int selectedDisplayIndex = 0;

    std::vector<Profile> profiles;
    Profile workingProfile;
    int selectedProfileIndex = -1;       
    
    UINT toggleHotkey = 0;
    UINT nextProfileHotkey = 0;
    UINT previousProfileHotkey = 0;
    
    bool loopProfiles = false;
    bool startMinimized = false;
    bool minimizeToTray = true; // Default on, the most common use-case.
    bool launchOnStartup = false;
    bool applyProfileOnLaunch = false;
    std::wstring lastSelectedProfileName = L"";       
    
    Profile simpleProfile;
    
    bool gammaRampFailed = false;
    float lastRamp[GammaConstants::RAMP_SIZE] = {};
    
    void SyncGammaToState()
    {
        if (state.IsGammaEnabled())
        {
            // Re-enable gamma, apply current working values.
            // @TODO: Should simple mode use the workingProfile?
            if (state.IsAdvancedModeEnabled())
            {
                // Advanced mode applies workingProfile.
                GammaManager::ApplyProfile(workingProfile, selectedDisplayIndex);
            }
            else
            {
                // Simple mode applies simple profile.
                GammaManager::ApplyProfile(simpleProfile, selectedDisplayIndex);
            }
        }
        else
        {
            GammaManager::ResetDisplay(selectedDisplayIndex);
        }
    }

    bool HasSelectedProfile()
    {
        return selectedProfileIndex >= 0 &&
            selectedProfileIndex < (int)profiles.size();
    }

    std::wstring GetStatusText()
    {
        // Base text shows current on/off state.
        std::wstring statusText = state.IsGammaEnabled() ? L"GammaHotkey - On" : L"GammaHotkey - Off";
        
        // Append profile name if one is selected, providing context to user.
        if (state.IsAdvancedModeEnabled() && selectedProfileIndex >= 0 && selectedProfileIndex < (int)profiles.size())
        {
            const std::wstring& profileName = profiles[selectedProfileIndex].name;
            if (!profileName.empty())
            {
                statusText += L" (" + profileName + L")";
            }
        }
        
        return statusText;
    }
}
