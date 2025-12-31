// Copyright (c) 2025 Max Godman

// Application entry point and window management.
// This is the core Win32 entry point and message loop for GammaHotkey.

/**
 * ARCHITECTURE OVERVIEW:
 * - Win32 window provides the container for ImGui rendering.
 * - DirectX 11 is used for hardware-accelerated rendering (ImGui backend).
 * - Message loop uses PeekMessage (non-blocking) to allow continuous ImGui rendering.
 * - Window is borderless, title bar and controls are drawn by ImGui for consistent styling.
 * - AppGlobals/AppState and UIGlobals/UIState provides centralized app and UI globals and state
 *   management via App and UI namespaces. State global objects accessible via App/UI::state.
 * - Functional duties are segregated into managers and utils.
 * 
 * DESIGN DECISIONS:
 * - Single instance enforcement:
 *   Uses mutex based on executable path to prevent accidental double-launches of the same .exe,
 *   while allowing multiple instances via renamed/relocated copies.
 * - DPI awareness:
 *   Per-monitor DPI V2 for proper scaling across multiple monitors.
 * - Keyboard hook instead of RegisterHotKey:
 *   Allows binding any key including systems keys such as Alt/F10, chorded inputs are not supported.
 * - Simple and Advanced modes:
 *   Simple mode by default offers frictionless basic functionality, for users looking to quickly
 *   set up gamma adjustments with a toggle hotkey.
 *   Advanced mode offers a profile based setup, for users who want to toggle or cycle through
 *   multiple profiles. For example, one hotkey to toggle on/off, two to cycle through profiles,
 *   allowing the user to set up incremental profiles and adjust up/down as needed.
 */

#include "framework.h"
#include "Resource.h"
#include "AppGlobals.h"
#include "UIGlobals.h"
#include "GammaHotkeyTypes.h"
#include "ConfigManager.h"
#include "GammaManager.h"
#include "HotkeyManager.h"
#include "DisplayManager.h"
#include "StartupManager.h"
#include "SystemTrayManager.h"
#include "ImGui_Integration.h"
#include "UI_Shared.h"

// Explicitly link libraries.
// This seems to be handled automatically in VS by CoreLibraryDependencies in the project properties.
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool EnforceSingleInstance();
ATOM RegisterMainWindowClass(const HINSTANCE hInstance);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void ShowMainWindow(const HWND hWnd);
void HideMainWindow(const HWND hWnd);
bool RenderImGuiFrame();

// Global instance variables.
HINSTANCE hInst;
WCHAR szTitle[AppConstants::MAX_LOADSTRING];
WCHAR szWindowClass[AppConstants::MAX_LOADSTRING];

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR lpCmdLine,
                     _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Enforce only a single instance of the application by matching mutex.
    if (!EnforceSingleInstance())
        return 0;

    // Enable per-monitor DPI awareness V2.
    // This enables automatic DPI scaling when moving the window between monitors with different DPIs.
    // V2 specifically enables WM_DPICHANGED messages for dynamic rescaling during window moves.
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, AppConstants::MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_GAMMAHOTKEY, szWindowClass, AppConstants::MAX_LOADSTRING);
    RegisterMainWindowClass(hInstance);

    hInst = hInstance;

    const HWND hWnd = CreateWindowW(szWindowClass, szTitle,
        WS_POPUP | WS_THICKFRAME,  // Borderless but resizable.
        CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, // Created with zero window size, updated to desired size later.
        nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return FALSE;
    
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));

    // Main message loop.
    // Uses PeekMessage instead of GetMessage.
    // PeekMessage is non-blocking, allowing continuous ImGui rendering even when there are no Windows messages.
    while (msg.message != WM_QUIT)
    {
        // Process all pending Windows messages first.
        if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        // When no messages, render ImGui frame.
        else
        {
            // When no messages, render ImGui frame.
            RenderImGuiFrame();
        }
    }

    return (int)msg.wParam;
}

/**
 * @brief Prevents accidental double-launches by enforcing a single instance per executable path.
 *
 * Uses a mutex based on the full executable path.
 *
 * Allows intentional use of multiple copies:
 * - Same exe + different location = Can run (different path).
 * - Renamed exe + same location = Can run (different name in path).
 * - Same exe + same location = Only one instance (same path).
 *
 * This prevents user confusion from accidentally launching twice via double-click,
 * while allowing power users to run multiple instances if desired.
 */
bool EnforceSingleInstance()
{
    wchar_t exePath[MAX_PATH];
    if (!GetModuleFileNameW(nullptr, exePath, MAX_PATH))
        return true; // Fail open.

    // Create mutex name from path.
    std::wstring mutexName = L"GammaHotkey_";
    mutexName += exePath;

    // Mutex names can't contain backslashes, colons, or slashes.
    for (wchar_t& c : mutexName)
    {
        if (c == L'\\' || c == L':' || c == L'/')
            c = L'_';
    }

    static HANDLE hMutex = nullptr;
    hMutex = CreateMutexW(nullptr, FALSE, mutexName.c_str());
    if (!hMutex)
        return true;

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        // Another instance from this exact location is already running.
        MessageBoxW(
            nullptr,
            VER_PRODUCTNAME_W L" is already running from this location.\n\n"
            L"Check your system tray for the " VER_PRODUCTNAME_W L" icon.",
            L"Already Running",
            MB_OK | MB_ICONINFORMATION);

        CloseHandle(hMutex);
        hMutex = nullptr;
        return false;
    }

    return true;
}

ATOM RegisterMainWindowClass(const HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW; // @TODO: Do we want to redraw when width/height changes?
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GAMMAHOTKEY));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);  // Black background to match ImGui. @TODO: Unsure if this actually works.
    wcex.lpszMenuName = nullptr;  // No Win32 menu.
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    return RegisterClassExW(&wcex);
}

void ShowMainWindow(const HWND hWnd)
{
    ShowWindow(hWnd, SW_SHOW);
    ShowWindow(hWnd, SW_RESTORE);
    SetForegroundWindow(hWnd);
}

void HideMainWindow(const HWND hWnd)
{
    ShowWindow(hWnd, SW_HIDE);
}

/**
 * @brief Handles rendering a new ImGui frame.
 * New ImGui frame -> Build our ImGui UI -> Render the ImGui frame.
 */
bool RenderImGuiFrame()
{
    if (!g_ImGuiRenderer || !g_ImGuiRenderer->IsInitialized())
    {
        return false; // Not ready to render.
    }

    // Start the ImGui frame, handle input.
    g_ImGuiRenderer->NewFrame();

    // Build the application UI using ImGui.
    RenderMainUI();

    // Now render it.
    g_ImGuiRenderer->Render();
    return true;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // ImGui backend needs to first look at all messages to track mouse/keyboard state.
    // If ImGui handles it (returns true), we don't process further to avoid conflicts.
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
        return true;

    switch (message)
    {
    case WM_CREATE: // @TODO: Review what belongs here, and what should be shifted elsewhere.
    {
        if (!hWnd) return -1;

        // We currently only ever expect one window to be created, so we assume this is the main window.
        App::mainWindow = hWnd;

        // Initialize ImGui renderer for the UI.
        g_ImGuiRenderer = new ImGuiRenderer();
        if (!g_ImGuiRenderer->Initialize(hWnd))
        {
            MessageBoxW(hWnd, L"Failed to initialize ImGui!", L"Error", MB_OK | MB_ICONERROR);
            return -1;
        }

        // Initialize COM, needed for startup shortcuts.
        HRESULT hr = CoInitialize(nullptr);
        if (FAILED(hr))
        {
            // @TODO:   What exactly are the implications of failure here?
            //          Also handle CoUninitialize more safely with a similar check?
            return -1;
        }

        // Enumerate displays, required before config load validates display index.
        DisplayManager::EnumerateDisplays();

        // Load config and register hotkeys.
        ConfigManager::Load();
        App::state.SetConfigInitialized(true); // Mark initialized, so we can check if config data is ready.
        HotkeyManager::RegisterAll(hWnd);
        
        // Window was created with zero size, now update it.
        App::SyncWindowSizeToState();
        
        // Validate selected monitor index, required after config load.
        if (App::selectedDisplayIndex < 0 || App::selectedDisplayIndex >= (int)App::displays.size())
        {
            App::selectedDisplayIndex = 0; // Fallback to 0 if invalid.
        }
        
        // Check startup shortcut status.
        App::launchOnStartup = StartupManager::IsEnabled();

        // Initialize lastRamp, we have not applied a ramp yet, and we are assumed in a default state right now.
        // @TODO:   We could use GetDeviceGammaRamp() to populate this, allowing us to update the graph with the current ramp.
        //          If the user has a gamma ramp set already for whatever reason this could be helpful, for now this is unnecessary.
        for (int i = 0; i < GammaConstants::RAMP_SIZE; ++i)
            App::lastRamp[i] = i / 255.0f;

        // Add system tray icon, do this early enough to later receive an update as part of initialization.
        SystemTrayManager::AddIcon(hWnd);

        // Handle advanced and simple mode profile initialization as desired by the settings.
        if (App::state.IsAdvancedModeEnabled() && App::HasSelectedProfile())
        {
            // Advanced mode, requires a valid selected profile.
            // Load the selected profile into working copy and sync UI.
            App::workingProfile = App::profiles[App::selectedProfileIndex];
            SyncUIWithCurrentProfile();
            
            // Apply profile on launch only if "Toggle on when launched" is enabled.
            if (App::applyProfileOnLaunch)
            {
                App::state.SetGammaEnabled(true);
                App::SyncGammaToState();
            }
        }
        else if (!App::state.IsAdvancedModeEnabled() && App::applyProfileOnLaunch)
        {
            // Simple mode, with "Toggle on when launched" enabled.
            App::state.SetGammaEnabled(true);
            App::SyncGammaToState();
        }
        
        // Ensure UI is synced after any state changes.
        UI::SyncUIToState();

        // Show or hide window based on settings.
        if (App::startMinimized)
        {
            HideMainWindow(hWnd);
        }
        else
        {
            // @TODO:   There is a flicker when the window appears for the first time.
            //          It looks like ImGui hasn't rendered yet for a frame. Can this be resolved reasonably?
            UpdateWindow(hWnd);
            ShowWindow(hWnd, SW_SHOW);
        }

        break;
    }

    case WM_CLOSE:
        // Check if we should minimize to tray instead of closing.
        if (App::minimizeToTray)
        {
            ShowWindow(hWnd, SW_HIDE);
            return 0;  // Don't proceed to default handler.
        }
        // Otherwise, allow normal close (will call DestroyWindow).
        return DefWindowProc(hWnd, message, wParam, lParam);

    case WM_DESTROY:
        // @TODO:   Can we handle things better if the process is killed?
        //          Confirm what happens when shutting down the PC with the app running.
        //          We try to avoid saving excessively, like every time a slider value is changed,
        //          but we also save excessively here every time the app is closed, handle this better.
        // Save config before closing.
        ConfigManager::Save();
        
        // Reset gamma to default before closing.
        GammaManager::ResetDisplay(App::selectedDisplayIndex);
        
        HotkeyManager::UnregisterAll(hWnd);
        SystemTrayManager::RemoveIcon();
        
        if (g_ImGuiRenderer)
        {
            g_ImGuiRenderer->Shutdown();
            delete g_ImGuiRenderer;
            g_ImGuiRenderer = nullptr;
        }
        
        // @TODO: This may be unsafe if we failed to CoInitialize()? Investigate.
        CoUninitialize();
        PostQuitMessage(0);
        break;

    case WM_SIZING:
        // @TODO: Investigate best practices around handling WM_SIZING and WM_SIZE, uncertain of this approach.
        // User is actively resizing, force immediate render.
        RenderImGuiFrame();
        return TRUE;

    case WM_SIZE:
        if (g_ImGuiRenderer && wParam != SIZE_MINIMIZED)
        {
            g_ImGuiRenderer->OnResize(LOWORD(lParam), HIWORD(lParam));
            // Don't render immediately, let the next frame handle it.
            // Immediate rendering during maximize causes nested ImGui frames and crashes.
        }
        break;

    case WM_DPICHANGED:
    {
        // Handle DPI change when window moves between monitors with different DPI.
        const UINT newDpi = HIWORD(wParam);
        
        // Update ImGui DPI scaling.
        if (g_ImGuiRenderer)
        {
            g_ImGuiRenderer->OnDpiChanged(newDpi);
        }
        
        // Resize window to suggested size from Windows.
        const RECT* suggestedRect = (RECT*)lParam;
        SetWindowPos(hWnd, nullptr,
                    suggestedRect->left, suggestedRect->top,
                    suggestedRect->right - suggestedRect->left,
                    suggestedRect->bottom - suggestedRect->top,
                    SWP_NOZORDER | SWP_NOACTIVATE);
        break;
    }

    case WM_ERASEBKGND:
        // Don't erase background, ImGui will draw everything.
        return 1;

    case WM_ENTERSIZEMOVE:
    case WM_EXITSIZEMOVE:
        InvalidateRect(hWnd, nullptr, FALSE);
        break;

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case SystemTrayIDs::ID_SHOW:
            ShowMainWindow(hWnd);
            return 0;
        case SystemTrayIDs::ID_TOGGLE:
            // Toggle gamma on/off, handle it the same as the toggle hotkey.
            // @TODO: This is a bit of a hack, can we handle this better?
            HotkeyManager::HandleHotkey(HotkeyIDs::TOGGLE);
            return 0;
        case SystemTrayIDs::ID_EXIT:
            DestroyWindow(hWnd);
            return 0;
        }
        break;
    }

    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_MINIMIZE)
        {
            if (App::minimizeToTray)
            {
                HideMainWindow(hWnd);
                return 0;
            }
        }
        else if ((wParam & 0xFFF0) == SC_CLOSE)
        {
            if (App::minimizeToTray)
            {
                HideMainWindow(hWnd);
                return 0;
            }
        }
        else if ((wParam & 0xFFF0) == SC_KEYMENU)
        {
            // Block system keys from activating menu bar.
            return 0;
        }
        return DefWindowProc(hWnd, message, wParam, lParam);

    case WM_SYSKEYDOWN:
        // Allow system keys such as Alt to be captured for hotkey binding.
        OnHotkeyCapture((UINT)wParam);
        // Block them from activating menus.
        return 0;

    case WM_MENUCHAR:
        // Block F10 and other menu accelerators.
        // Return MNC_CLOSE to prevent beep sound.
        return MAKELRESULT(0, MNC_CLOSE);

    case SystemTrayIDs::WM_ICON:
        if (lParam == WM_LBUTTONDOWN)
        {
            ShowMainWindow(hWnd);
        }
        else if (lParam == WM_RBUTTONDOWN)
        {
            SystemTrayManager::ShowContextMenu(hWnd);
        }
        return 0;

    case WM_KEYDOWN:
        // Capture hotkey for ImGui popup.
        // Actual hotkey handling is done by keyboard hook.
        OnHotkeyCapture((UINT)wParam);
        return 0;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    
    return 0;
}
