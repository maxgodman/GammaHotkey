// Copyright (c) 2025 Max Godman

#include "framework.h"
#include "HotkeyManager.h"
#include "AppGlobals.h"
#include "UIGlobals.h"
#include "GammaHotkeyTypes.h"
#include "ProfileManager.h"
#include "UI_Shared.h"

namespace HotkeyManager
{
    // Keyboard hook handle and target window.
    static HHOOK s_keyboardHook = nullptr;
    static HWND s_targetWindow = nullptr;
    
    /**
     * Low-level keyboard hook callback.
     * Called by Windows for every keystroke system-wide (before any application sees it).
     * We check if it matches our hotkeys, and consume it if so.
     */
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
    {
        if (nCode == HC_ACTION)
        {
            KBDLLHOOKSTRUCT* pKeyboard = (KBDLLHOOKSTRUCT*)lParam;
            
            // Only handle key down events (ignore key up).
            // WM_SYSKEYDOWN is for system keys such as Alt.
            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
            {
                const UINT vk = pKeyboard->vkCode;
                
                // Check if this key matches any of our registered hotkeys.
                bool handled = false;
                
                // Check toggle hotkey.
                if (App::toggleHotkey != 0 && vk == App::toggleHotkey)
                {
                    HandleHotkey(HotkeyIDs::TOGGLE);
                    handled = true;
                }
                // Check previous profile hotkey.
                else if (App::previousProfileHotkey != 0 && vk == App::previousProfileHotkey)
                {
                    HandleHotkey(HotkeyIDs::PREVIOUS_PROFILE);
                    handled = true;
                }
                // Check next profile hotkey.
                else if (App::nextProfileHotkey != 0 && vk == App::nextProfileHotkey)
                {
                    HandleHotkey(HotkeyIDs::NEXT_PROFILE);
                    handled = true;
                }
                // Check profile hotkeys.
                else
                {
                    for (size_t i = 0; i < App::profiles.size(); ++i)
                    {
                        if (App::profiles[i].hotkey != 0 && vk == App::profiles[i].hotkey)
                        {
                            HandleHotkey(HotkeyIDs::PROFILE_BASE + (int)i);
                            handled = true;
                            break;
                        }
                    }
                }
                
                // If we handled the hotkey, consume it (don't pass to other apps).
                if (handled)
                {
                    return 1;
                }
            }
        }
        
        // Pass to next hook.
        return CallNextHookEx(s_keyboardHook, nCode, wParam, lParam);
    }
    
    void UnregisterAll(const HWND hwnd)
    {
        // Unhook keyboard hook.
        if (s_keyboardHook != nullptr)
        {
            UnhookWindowsHookEx(s_keyboardHook);
            s_keyboardHook = nullptr;
        }

        s_targetWindow = nullptr;
    }
    
    void RegisterAll(const HWND hwnd)
    {
        // Unregister existing hook if any.
        UnregisterAll(hwnd);
        
        // Setup low-level keyboard hook.
        s_targetWindow = hwnd;
        s_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(nullptr), 0);
        
        if (s_keyboardHook == nullptr)
        {
            // Hook setup failed.
            const DWORD error = GetLastError();
            wchar_t msg[256];
            swprintf_s(msg, L"Failed to setup keyboard hook. Error code: %d", error);
            MessageBoxW(hwnd, msg, L"Hotkey Error", MB_OK | MB_ICONERROR);
        }
    }
    
    void HandleHotkey(const int hotkeyId)
    {
        // Close any open context menus (e.g. System Tray menu).
        // This is needed because our keyboard hook consumes keys before Windows can close menus.
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
        }
        else if (hotkeyId >= HotkeyIDs::PROFILE_BASE && 
                 hotkeyId < HotkeyIDs::PROFILE_BASE + (int)App::profiles.size())
        {
            const int profileIndex = hotkeyId - HotkeyIDs::PROFILE_BASE;
            App::state.SetGammaEnabled(true); // Ensure enabled when profile triggered.
            ProfileManager::ApplyByIndex(profileIndex);
            SyncUIWithCurrentProfile();
        }
    }
}
