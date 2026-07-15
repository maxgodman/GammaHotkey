// Copyright (c) 2025 Max Godman

#include "framework.h"
#include "HotkeyManager.h"
#include "AppGlobals.h"
#include "UIGlobals.h"
#include "GammaHotkeyTypes.h"
#include "ProfileManager.h"
#include "UI_Shared.h"
#include <vector>

namespace HotkeyManager
{
    // IDs currently registered with Windows, tracked so we can unregister exactly
    // what we registered even if the profile list changes between calls.
    static std::vector<int> s_registeredIds;

    // Register a single hotkey, tracking it on success. vk == 0 means "unbound", skip.
    // Registration can fail if another application already owns the global hotkey; we
    // tolerate that silently (the binding simply won't fire) rather than stealing the
    // key system-wide, which is the cooperative behavior RegisterHotKey is designed for.
    static void RegisterOne(const HWND hwnd, const int id, const UINT vk)
    {
        if (vk == 0)
            return;

        // MOD_NOREPEAT: holding the key fires once, not repeatedly.
        if (RegisterHotKey(hwnd, id, MOD_NOREPEAT, vk))
            s_registeredIds.push_back(id);
    }

    void UnregisterAll(const HWND hwnd)
    {
        for (const int id : s_registeredIds)
            UnregisterHotKey(hwnd, id);
        s_registeredIds.clear();
    }

    void RegisterAll(const HWND hwnd)
    {
        // Clear any existing registrations first.
        UnregisterAll(hwnd);

        RegisterOne(hwnd, HotkeyIDs::TOGGLE, App::toggleHotkey);
        RegisterOne(hwnd, HotkeyIDs::PREVIOUS_PROFILE, App::previousProfileHotkey);
        RegisterOne(hwnd, HotkeyIDs::NEXT_PROFILE, App::nextProfileHotkey);

        for (size_t index = 0; index < App::profiles.size(); ++index)
            RegisterOne(hwnd, HotkeyIDs::PROFILE_BASE + (int)index, App::profiles[index].hotkey);
    }

    bool IsBindableKey(const UINT vk, const char** reasonForRejection)
    {
        switch (vk)
        {
        // Bare modifier keys cannot be fired on their own by RegisterHotKey.
        case VK_SHIFT:   case VK_LSHIFT:   case VK_RSHIFT:
        case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
        case VK_MENU:    case VK_LMENU:    case VK_RMENU:
        case VK_LWIN:    case VK_RWIN:
            if (reasonForRejection)
                *reasonForRejection = "Modifier keys (Shift, Ctrl, Alt, Win) can't be used on their own.";
            return false;

        // F12 is reserved by Windows for the debugger.
        case VK_F12:
            if (reasonForRejection)
                *reasonForRejection = "F12 is reserved by Windows and can't be used.";
            return false;

        default:
            return true;
        }
    }

    void HandleHotkey(const int hotkeyId)
    {
        // Close any open context menus (e.g. the system tray menu) so a hotkey press
        // while the menu is open doesn't leave it stuck open.
        if (App::mainWindow)
        {
            SendMessage(App::mainWindow, WM_CANCELMODE, 0, 0);
        }

        if (hotkeyId == HotkeyIDs::TOGGLE)
        {
            // Update state and sync.
            App::state.SetGammaEnabled(!App::state.IsGammaEnabled());
            App::SyncGammaToState();
            UI::SyncUIToState();
        }
        else if (hotkeyId == HotkeyIDs::PREVIOUS_PROFILE)
        {
            // If gamma is disabled, just enable it (don't cycle to different profile).
            if (!App::state.IsGammaEnabled())
            {
                App::state.SetGammaEnabled(true);
                App::SyncGammaToState();
            }
            else
            {
                // Gamma is already enabled, cycle to previous profile.
                ProfileManager::CycleProfile(-1);
                SyncUIWithCurrentProfile();
            }
            UI::SyncUIToState();
        }
        else if (hotkeyId == HotkeyIDs::NEXT_PROFILE)
        {
            // If gamma is disabled, just enable it (don't cycle to different profile).
            if (!App::state.IsGammaEnabled())
            {
                App::state.SetGammaEnabled(true);
                App::SyncGammaToState();
            }
            else
            {
                // Gamma is already enabled, cycle to next profile.
                ProfileManager::CycleProfile(1);
                SyncUIWithCurrentProfile();
            }
            UI::SyncUIToState();
        }
        else if (hotkeyId >= HotkeyIDs::PROFILE_BASE &&
                 hotkeyId < HotkeyIDs::PROFILE_BASE + (int)App::profiles.size())
        {
            const int profileIndex = hotkeyId - HotkeyIDs::PROFILE_BASE;
            App::state.SetGammaEnabled(true); // Ensure enabled when profile triggered.
            ProfileManager::ApplyByIndex(profileIndex);
            SyncUIWithCurrentProfile();
            UI::SyncUIToState();
        }
    }
}
