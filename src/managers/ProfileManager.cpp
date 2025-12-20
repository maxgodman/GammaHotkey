// Copyright (c) 2025 Max Godman

#include "framework.h"
#include "ProfileManager.h"
#include "AppGlobals.h"
#include "GammaManager.h"

namespace ProfileManager
{
    int FindByName(const std::wstring& name)
    {
        for (size_t i = 0; i < App::profiles.size(); ++i)
        {
            if (_wcsicmp(App::profiles[i].name.c_str(), name.c_str()) == 0)
                return (int)i;
        }
        return -1;
    }
    
    void ApplyByIndex(const int index)
    {
        if (index >= App::profiles.size()) return;
        
        App::workingProfile = App::profiles[index];
        App::selectedProfileIndex = index;
        GammaManager::ApplyProfile(App::workingProfile, App::selectedDisplayIndex);
    }
    
    bool ApplyByName(const std::wstring& name)
    {
        const int index = FindByName(name);
        if (index >= 0)
        {
            ApplyByIndex(index);
            return true;
        }
        return false;
    }
    
    void CycleProfile(const int direction)
    {
        if (App::profiles.empty()) return;
        
        int newIndex = App::selectedProfileIndex + direction;

        if (App::loopProfiles)
        {
            // Wrap around.
            if (newIndex < 0) 
                newIndex = (int)App::profiles.size() - 1;
            if (newIndex >= (int)App::profiles.size()) 
                newIndex = 0;
        }
        else
        {
            // Clamp to bounds.
            if (newIndex < 0) 
                newIndex = 0;
            if (newIndex >= (int)App::profiles.size()) 
                newIndex = (int)App::profiles.size() - 1;
        }

        if (newIndex != App::selectedProfileIndex)
        {
            ApplyByIndex(newIndex);
        }
    }
    
    void DeleteProfile(const int index)
    {
        if (index < 0 || index >= (int)App::profiles.size())
            return;
        
        App::profiles.erase(App::profiles.begin() + index);
        
        // Update selected profile index if needed.
        if (App::selectedProfileIndex == index)
        {
            App::workingProfile = Profile();
            App::selectedProfileIndex = -1;
        }
        else if (App::selectedProfileIndex > index)
        {
            App::selectedProfileIndex--;
        }
    }
}
