// Copyright (c) 2025 Max Godman

#include "framework.h"
#include "IconManager.h"
#include "SystemTrayManager.h"

extern HINSTANCE hInst;

namespace IconManager
{
    void UpdateAllIcons(const bool gammaEnabled)
    {
        // Update system tray icon.
        SystemTrayManager::UpdateIcon(gammaEnabled);
        
        // @TODO:   No longer updating these icons, this was to use it as an on/off indicator.
        //          Decide on what to do with this, remove it, optional feature?
        //          Would like to add the on/off icon indicator into the ImGui title bar which could replace the below.
        //          Should also replace the on/off icons, currently green/red circle, should probably be the app icon with an indicator on it.
        //// Update window/taskbar icon.
        //const UINT iconID = gammaEnabled ? IDI_ON : IDI_OFF;
        //const HICON hIcon = LoadIcon(hInst, MAKEINTRESOURCE(iconID));
        //
        //if (hIcon && App::mainWindow)
        //{
        //    // Update both small and large icons for window and taskbar.
        //    SendMessage(App::mainWindow, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        //    SendMessage(App::mainWindow, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        //}
    }
    
    void Initialize(const HWND hwnd, const bool gammaEnabled)
    {
        // Set initial icon state.
        UpdateAllIcons(gammaEnabled);
    }
}
