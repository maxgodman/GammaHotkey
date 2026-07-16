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
 * - RegisterHotKey for global hotkeys:
 *   Binds single keys (letters, digits, function keys including F10) as system-wide hotkeys.
 *   Bare modifier keys (Alt/Ctrl/Shift/Win alone) and F12 are not supported, see HotkeyManager.
 *   A low-level keyboard hook was deliberately avoided as it resembles a keylogger and triggers
 *   antivirus false positives. Chorded (modifier+key) inputs are not supported.
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
#include <windowsx.h>
#include <uxtheme.h>  // MARGINS.
#include <dwmapi.h>

// Explicitly link libraries.
// This seems to be handled automatically in VS by CoreLibraryDependencies in the project properties.
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "dwmapi.lib")

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

// Whether this thread's CoInitialize (in WM_CREATE) took a reference and therefore owns a
// matching CoUninitialize (in WM_DESTROY). See the COM handling in WndProc.
static bool s_comInitialized = false;

// The custom caption button (HTMINBUTTON/HTMAXBUTTON/HTCLOSE) currently pressed, or HTNOWHERE if
// none. Set on WM_NCLBUTTONDOWN and cleared on WM_NCLBUTTONUP so the action fires only when press
// and release land on the same button, matching native caption-button behavior.
static WPARAM s_pressedCaptionButton = HTNOWHERE;

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

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, AppConstants::MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_GAMMAHOTKEY, szWindowClass, AppConstants::MAX_LOADSTRING);
    RegisterMainWindowClass(hInstance);

    hInst = hInstance;

    const HWND hWnd = CreateWindowW(szWindowClass, szTitle,
        // Borderless (WS_POPUP) but resizable (WS_THICKFRAME); WM_NCCALCSIZE strips the visible frame
        // so the ImGui title bar owns the window. The sys-menu/min/max box styles add no chrome
        // without WS_CAPTION but re-enable OS behaviors: WS_MAXIMIZEBOX is required for Aero Snap, and
        // the trio restores the min/max/snap animations and the taskbar thumbnail menu.
        WS_POPUP | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, // Created with zero window size, updated to desired size later.
        nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return FALSE;
    
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));

    // Main message loop.
    // Uses PeekMessage (non-blocking) so ImGui can render continuously while the window
    // is visible. When the window is hidden or minimized to the tray there is nothing to
    // draw, so we block in WaitMessage instead of spinning, keeping idle CPU usage at zero.
    while (msg.message != WM_QUIT)
    {
        // Process all pending Windows messages first.
        if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else if (App::mainWindow && IsWindowVisible(App::mainWindow) && !IsIconic(App::mainWindow))
        {
            // Window is visible: render the next ImGui frame.
            RenderImGuiFrame();
        }
        else
        {
            // Window is hidden/minimized: sleep until the next message arrives.
            WaitMessage();
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
    // No class redraw styles: this window paints via the DirectX 11 swap chain every frame
    // (and forces a render during interactive resize, see WM_SIZING), never through GDI/WM_PAINT.
    // CS_HREDRAW/CS_VREDRAW only invalidate the client area for a GDI repaint on resize, which
    // this app swallows (WM_ERASEBKGND returns 1, WM_PAINT is unhandled), so they do nothing
    // useful here. The Dear ImGui DX11 example likewise registers its class without them.
    wcex.style = 0;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GAMMAHOTKEY));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    // Never painted: this app handles WM_ERASEBKGND, so the class brush is unused.
    // Kept as a harmless dark default in case the erase handling ever changes.
    wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.lpszMenuName = nullptr;  // No Win32 menu.
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    return RegisterClassExW(&wcex);
}

// Show the window without the white flash that appears when a never-composited (or previously
// hidden) window is shown: its DWM redirection surface starts blank and DWM composites that blank
// surface for one frame before our next render lands. Cloak the window (DWMWA_CLOAK) so DWM does
// not display it while it is shown, render one frame so the redirection surface holds the finished
// UI, then uncloak to reveal it already painted. Covers both first show and re-show from the tray.
static void ShowWindowCloaked(const HWND hWnd, const bool restore)
{
    BOOL cloaked = TRUE;
    DwmSetWindowAttribute(hWnd, 13 /* DWMWA_CLOAK */, &cloaked, sizeof(cloaked));

    ShowWindow(hWnd, SW_SHOW);
    if (restore)
        ShowWindow(hWnd, SW_RESTORE); // Un-minimize when re-shown from the tray.

    RenderImGuiFrame();

    cloaked = FALSE;
    DwmSetWindowAttribute(hWnd, 13 /* DWMWA_CLOAK */, &cloaked, sizeof(cloaked));
}

void ShowMainWindow(const HWND hWnd)
{
    ShowWindowCloaked(hWnd, true);
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
    case WM_CREATE:
    {
        if (!hWnd) return -1;

        // We currently only ever expect one window to be created, so we assume this is the main window.
        App::mainWindow = hWnd;

        // WM_NCCALCSIZE removed the standard frame so the ImGui title bar owns the window, which also
        // dropped the DWM drop shadow. Extending the frame one pixel back into the client area makes
        // DWM render the standard shadow (and cooperate with snap animations) again; the opaque ImGui
        // client paints over that one-pixel sliver so it is never visible.
        const MARGINS shadowMargins = { 0, 0, 0, 1 };
        DwmExtendFrameIntoClientArea(hWnd, &shadowMargins);

        // Prefer rounded corners on Windows 11. DWMWA_WINDOW_CORNER_PREFERENCE (33) and DWMWCP_ROUND
        // (2) live behind a newer NTDDI_VERSION than this project targets (see targetver.h), so pass
        // the numeric values directly; DwmSetWindowAttribute returns an ignored error where unsupported.
        const DWORD cornerPreferenceRound = 2; // DWMWCP_ROUND
        DwmSetWindowAttribute(hWnd, 33 /* DWMWA_WINDOW_CORNER_PREFERENCE */,
            &cornerPreferenceRound, sizeof(cornerPreferenceRound));

        // Initialize ImGui renderer for the UI.
        g_ImGuiRenderer = new ImGuiRenderer();
        if (!g_ImGuiRenderer->Initialize(hWnd))
        {
            MessageBoxW(hWnd, L"Failed to initialize ImGui!", L"Error", MB_OK | MB_ICONERROR);
            return -1;
        }

        // Initialize COM on this (UI) thread. COM is only used by StartupManager, to write the
        // "launch on startup" shortcut via IShellLink; nothing else in the app needs it. A failure
        // is therefore not fatal - we keep running and only that optional feature degrades (its
        // CoCreateInstance fails so the shortcut just isn't created), rather than tearing down the
        // whole app over it. Record whether we actually took a reference so WM_DESTROY balances it
        // correctly: S_OK and S_FALSE both need a matching CoUninitialize (both are SUCCEEDED),
        // whereas RPC_E_CHANGED_MODE (COM already initialized in a different mode) took no
        // reference and must not be uninitialized.
        const HRESULT hr = CoInitialize(nullptr);
        s_comInitialized = SUCCEEDED(hr);

        // Enumerate displays, required before config load validates display index.
        DisplayManager::EnumerateDisplays();

        // Load config and register hotkeys.
        ConfigManager::Load();
        App::state.SetConfigInitialized(true); // Mark initialized, so we can check if config data is ready.
        HotkeyManager::RegisterAll(hWnd);
        
        // Window was created with zero size, now update it.
        App::SyncWindowSizeToState();
        
        // Validate selected monitor index, required after config load.
        if (App::selectedDisplayIndex < -1 || App::selectedDisplayIndex >= (int)App::displays.size())
        {
            App::selectedDisplayIndex = 0; // Fallback to 0 if invalid.
        }
        
        // Check startup shortcut status.
        App::launchOnStartup = StartupManager::IsEnabled();

        // Seed lastRamp from the display's current gamma ramp so the curve graph reflects reality on
        // launch, in case another tool (or a prior session) left a non-default ramp applied. When the
        // target is "all displays" (-1) we read display 0 as representative, mirroring how GammaManager
        // opens a DC via CreateDC on the device name. GetDeviceGammaRamp gives WORD[3][256] per channel
        // (0-65535); we average the channels into the single 0..1 curve the preview consumes, inverting
        // how BuildGammaRamp stores it (identical channels round-trip exactly). Falls back to the linear
        // identity if there is no display or the read fails.
        bool seededFromDevice = false;
        if (!App::displays.empty())
        {
            const int readIndex = (App::selectedDisplayIndex >= 0) ? App::selectedDisplayIndex : 0;
            const HDC hdc = CreateDC(NULL, App::displays[readIndex].deviceName.c_str(), NULL, NULL);
            if (hdc)
            {
                WORD currentRamp[3][GammaConstants::RAMP_SIZE];
                if (GetDeviceGammaRamp(hdc, currentRamp))
                {
                    for (int index = 0; index < GammaConstants::RAMP_SIZE; ++index)
                    {
                        const float channelAverage = (currentRamp[0][index] + currentRamp[1][index] + currentRamp[2][index]) / 3.0f;
                        App::state.lastRamp[index] = channelAverage / GammaConstants::RAMP_MAX;
                    }
                    seededFromDevice = true;
                }
                DeleteDC(hdc);
            }
        }

        if (!seededFromDevice)
        {
            // No display available or the read failed, assume the default (linear) state.
            for (int index = 0; index < GammaConstants::RAMP_SIZE; ++index)
                App::state.lastRamp[index] = index / 255.0f;
        }

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
            // Render one complete ImGui frame before the first show, so the swap chain already
            // holds the finished UI when the window is revealed. The window is already sized
            // (SyncWindowSizeToState above resized the swap chain).
            RenderImGuiFrame();

            // Show the window without the startup white flash (see ShowWindowCloaked).
            ShowWindowCloaked(hWnd, false);
        }

        break;
    }

    case WM_QUERYENDSESSION:
        // Windows is asking whether it can end the session (shutdown/logoff/restart).
        return TRUE; // Allow it.

    case WM_ENDSESSION:
        // The session is ending and the process may be terminated without WM_DESTROY.
        // Persist settings and restore gamma now so nothing is lost. Guard the save on
        // IsConfigInitialized for the same reason as WM_DESTROY: never write in-memory
        // defaults over the user's config if it was somehow never loaded.
        if (wParam)
        {
            if (App::state.IsConfigInitialized())
                ConfigManager::Save();
            GammaManager::ResetDisplay(App::selectedDisplayIndex);
        }
        return 0;

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
        // Persist settings on a normal close. This is not an "excessive" save: the config is
        // already written after each user action, but a few runtime-only changes are not saved
        // anywhere else (most notably the selected profile after cycling it with the next/previous
        // hotkeys), so this final save captures them. ConfigManager::Save writes a small UTF-8 .ini
        // via a temp file and an atomic rename - cheap enough to do once on exit, so a dirty-flag
        // is not worth its complexity here.
        //
        // A hard process kill (TerminateProcess / End task) cannot be intercepted, so at worst the
        // on-disk config is missing runtime-only changes made since the last per-action save; the
        // atomic rename means no partial/corrupt file results. PC shutdown and logoff are handled
        // by WM_ENDSESSION above, which runs the same save for the case where WM_DESTROY is not
        // delivered.
        //
        // Guard on IsConfigInitialized so an early teardown never overwrites the user's real config
        // with in-memory defaults: WM_DESTROY also fires when WM_CREATE returns -1 (e.g. renderer
        // init failed before the config was loaded).
        if (App::state.IsConfigInitialized())
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
        
        // Balance the CoInitialize from WM_CREATE, but only when it actually took a reference
        // (S_OK/S_FALSE). If it failed - including RPC_E_CHANGED_MODE, where COM was already
        // initialized in another mode and we took no reference - we must not uninitialize. This
        // guard also matters because WM_DESTROY runs when WM_CREATE returns -1 (e.g. renderer init
        // failed), before COM was ever initialized.
        if (s_comInitialized)
        {
            CoUninitialize();
            s_comInitialized = false;
        }
        PostQuitMessage(0);
        break;

    case WM_GETMINMAXINFO:
    {
        // Constrain the maximized window to the monitor's work area so it doesn't cover the
        // taskbar. A borderless (WS_POPUP) window otherwise maximizes over the whole monitor;
        // WM_NCCALCSIZE alone can only shrink the client rect, not the window rect, leaving the
        // strip over the taskbar as undrawn non-client area (the black/blank taskbar symptom).
        // ptMaxPosition/ptMaxSize are relative to the monitor origin, so use the work-area
        // offset and size (this also handles a taskbar docked to the top or a side).
        const HMONITOR hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO monInfo = { sizeof(MONITORINFO) };
        GetMonitorInfo(hMon, &monInfo);
        const RECT& work = monInfo.rcWork;
        const RECT& mon = monInfo.rcMonitor;

        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        mmi->ptMaxPosition.x = work.left - mon.left;
        mmi->ptMaxPosition.y = work.top - mon.top;
        mmi->ptMaxSize.x = work.right - work.left;
        mmi->ptMaxSize.y = work.bottom - work.top;
        return 0;
    }

    case WM_NCCALCSIZE:
    {
        if (wParam == TRUE)
        {
            NCCALCSIZE_PARAMS* params = (NCCALCSIZE_PARAMS*)lParam;

            // Check if maximized by looking at window style.
            const LONG windowStyle = GetWindowLong(hWnd, GWL_STYLE);
            const bool maximized = (windowStyle & WS_MAXIMIZE) != 0;

            if (maximized)
            {
                // Make the client area fill the whole (borderless) window rect. WM_GETMINMAXINFO
                // has already constrained that rect to the monitor work area, so the client ends
                // up exactly the work area with no undrawn frame over the taskbar.
                const HMONITOR hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
                MONITORINFO monInfo = { sizeof(MONITORINFO) };
                GetMonitorInfo(hMon, &monInfo);
                params->rgrc[0] = monInfo.rcWork;
                return 0;
            }
            else
            {
                // When windowed, let the default calculation happen, but then
                // remove the top non-client area that causes the white bar.
                const LRESULT result = DefWindowProc(hWnd, message, wParam, lParam);

                // Get the window's border thickness
                const int borderThickness = GetSystemMetrics(SM_CXSIZEFRAME) +
                    GetSystemMetrics(SM_CXPADDEDBORDER);

                // Restore the top edge (removes the title bar space).
                params->rgrc[0].top = params->rgrc[0].top - borderThickness;

                return result;
            }
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    case WM_NCHITTEST:
    {
        POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(hWnd, &point);

        // Custom caption buttons take priority over both the frame and the caption, so they stay
        // fully clickable to the very edge and the maximize button's HTMAXBUTTON keeps driving the
        // Windows 11 snap-layouts flyout. Rects are published by RenderTitleBar each frame.
        const auto& tb = UI::state.titleBar;
        if (tb.valid)
        {
            if (PtInRect(&tb.closeButton, point)) return HTCLOSE;
            if (PtInRect(&tb.maxButton, point)) return HTMAXBUTTON;
            if (PtInRect(&tb.minButton, point)) return HTMINBUTTON;
        }

        // Otherwise resolve the frame via the default proc. With the frame removed by WM_NCCALCSIZE
        // this is HTCLIENT almost everywhere; if it does report a frame region, honor it unchanged.
        const LRESULT hit = DefWindowProc(hWnd, message, wParam, lParam);
        if (hit != HTCLIENT)
            return hit;

        RECT rect;
        GetClientRect(hWnd, &rect);

        const bool maximized = (GetWindowLong(hWnd, GWL_STYLE) & WS_MAXIMIZE) != 0;

        // Resize handles along the borders, but never on a maximized window (which cannot be
        // resized). These are tested before the caption so the top edge still resizes rather than
        // dragging.
        if (!maximized)
        {
            const int borderWidth = GetSystemMetrics(SM_CXSIZEFRAME) +
                GetSystemMetrics(SM_CXPADDEDBORDER);
            const int borderHeight = GetSystemMetrics(SM_CYSIZEFRAME) +
                GetSystemMetrics(SM_CXPADDEDBORDER);

            const bool onLeft = point.x < borderWidth;
            const bool onRight = point.x >= rect.right - borderWidth;
            const bool onTop = point.y < borderHeight;
            const bool onBottom = point.y >= rect.bottom - borderHeight;

            if (onTop && onLeft) return HTTOPLEFT;
            if (onTop && onRight) return HTTOPRIGHT;
            if (onBottom && onLeft) return HTBOTTOMLEFT;
            if (onBottom && onRight) return HTBOTTOMRIGHT;
            if (onLeft) return HTLEFT;
            if (onRight) return HTRIGHT;
            if (onTop) return HTTOP;
            if (onBottom) return HTBOTTOM;
        }

        // The drag strip reports HTCAPTION, so Windows runs its own move loop: the source of Aero
        // Snap, taskbar peek, window-shake and drag-off-maximize that a self-managed SetWindowPos
        // drag could not provide. The About button and content right of dragRight stay HTCLIENT, so
        // ImGui keeps those clicks; this holds when maximized too, so the caption can still be
        // grabbed to drag the window off its maximized state.
        if (tb.valid &&
            point.y >= 0 && point.y < tb.captionBottom &&
            point.x >= 0 && point.x < tb.dragRight)
        {
            return HTCAPTION;
        }

        return HTCLIENT;
    }

    case WM_NCLBUTTONDOWN:
        // A press on a custom caption button: remember it and swallow the message so DefWindowProc
        // does not run its own (now frameless) caption-button tracking. The action fires on
        // WM_NCLBUTTONUP only if the release lands on the same button. Any other non-client press
        // (notably HTCAPTION) falls through to DefWindowProc, which starts the window-move loop.
        if (wParam == HTMINBUTTON || wParam == HTMAXBUTTON || wParam == HTCLOSE)
        {
            s_pressedCaptionButton = wParam;
            return 0;
        }
        return DefWindowProc(hWnd, message, wParam, lParam);

    case WM_NCLBUTTONUP:
        // Complete a caption-button click: run the action only when the release is over the same
        // button that was pressed. Minimize to the taskbar, toggle maximize/restore, and route
        // close through WM_CLOSE so the minimize-to-tray setting still applies.
        if (s_pressedCaptionButton != HTNOWHERE)
        {
            const WPARAM pressed = s_pressedCaptionButton;
            s_pressedCaptionButton = HTNOWHERE;
            if (wParam == pressed)
            {
                if (pressed == HTMINBUTTON)
                    ShowWindow(hWnd, SW_MINIMIZE);
                else if (pressed == HTMAXBUTTON)
                    ShowWindow(hWnd, IsZoomed(hWnd) ? SW_RESTORE : SW_MAXIMIZE);
                else if (pressed == HTCLOSE)
                    PostMessage(hWnd, WM_CLOSE, 0, 0);
            }
            return 0;
        }
        return DefWindowProc(hWnd, message, wParam, lParam);

    case WM_LBUTTONUP:
        // Clear any caption-button press that ended in the client area (e.g. pressed a button then
        // dragged off it before releasing) so it cannot mis-fire on a later non-client release.
        s_pressedCaptionButton = HTNOWHERE;
        return DefWindowProc(hWnd, message, wParam, lParam);

    case WM_SIZING:
        // An interactive border drag runs a modal resize loop inside Windows that blocks our
        // main PeekMessage loop, so nothing would repaint until the drag ends. Render inline
        // here to keep the UI live while the user drags. This is safe from nested ImGui frames
        // because WM_SIZING is only sent by that drag loop, never re-entrantly from within a
        // frame. Returning TRUE is the documented result for an app that processes WM_SIZING.
        RenderImGuiFrame();
        return TRUE;

    case WM_SIZE:
        if (g_ImGuiRenderer && wParam != SIZE_MINIMIZED)
        {
            // Only resize the swap-chain buffers here; deliberately do not render. WM_SIZE can
            // arrive synchronously from within an in-progress ImGui frame (RenderMainUI calls
            // SyncWindowSizeToState -> SetWindowPos on a Simple/Advanced mode change), so calling
            // NewFrame again would nest frames and crash. The next main-loop frame, or the
            // WM_SIZING render during an interactive drag, paints at the new size.
            g_ImGuiRenderer->OnResize(LOWORD(lParam), HIWORD(lParam));
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

    case WM_DISPLAYCHANGE:
    {
        // A monitor was added/removed or the resolution changed. Re-enumerate displays and
        // try to preserve the current selection by device name so gamma keeps targeting the
        // same physical monitor even if indices shifted.
        std::wstring previousDeviceName;
        if (App::selectedDisplayIndex >= 0 && App::selectedDisplayIndex < (int)App::displays.size())
            previousDeviceName = App::displays[App::selectedDisplayIndex].deviceName;

        DisplayManager::EnumerateDisplays();

        if (App::selectedDisplayIndex != -1) // -1 means "all displays", which stays valid.
        {
            int newIndex = -1;
            if (!previousDeviceName.empty())
            {
                for (int index = 0; index < (int)App::displays.size(); ++index)
                {
                    if (App::displays[index].deviceName == previousDeviceName)
                    {
                        newIndex = index;
                        break;
                    }
                }
            }

            if (newIndex >= 0)
                App::selectedDisplayIndex = newIndex;
            else
                App::selectedDisplayIndex = App::displays.empty() ? 0 : 0; // Fall back to the first display.
        }

        // Re-apply current gamma so it reflects on the (possibly changed) selected display.
        App::SyncGammaToState();
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
            App::ToggleGamma();
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

    case WM_HOTKEY:
        // A registered global hotkey fired. wParam is the hotkey ID.
        HotkeyManager::HandleHotkey((int)wParam);
        return 0;

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
