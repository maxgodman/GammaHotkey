// Copyright (c) 2025 Max Godman

#include "framework.h"
#include "ImGui_Integration.h"
#include "AppGlobals.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Global instance.
ImGuiRenderer* g_ImGuiRenderer = nullptr;

ImGuiRenderer::ImGuiRenderer()
{
}

ImGuiRenderer::~ImGuiRenderer()
{
    Shutdown();
}

bool ImGuiRenderer::Initialize(const HWND hwnd)
{
    m_hwnd = hwnd;

    // Create D3D11 device.
    if (!CreateDeviceD3D(hwnd))
        return false;

    // Setup Dear ImGui context.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;  // Disable .ini file saving/loading.

    // Setup Platform/Renderer backends.
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(m_pd3dDevice, m_pd3dDeviceContext);

    // Apply custom style.
    ApplyImGuiStyle();

    // Apply DPI scaling after style, so it scales everything. App::GetDpiScale() is the shared
    // source for the factor (hwnd is already published as App::mainWindow by the time WM_CREATE
    // constructs the renderer), so the style metrics scale by exactly the same number the UI code
    // uses for its own pixel literals.
    const float scale = App::GetDpiScale();
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(scale);   // Scale style metrics (paddings/spacings/borders).
    // Rasterize the font crisp at the actual DPI. FontScaleDpi is ImGui 1.92's per-monitor
    // font scale factor; the DX11 backend advertises ImGuiBackendFlags_RendererHasTextures,
    // so the atlas is re-baked at the requested size automatically. Not io.FontGlobalScale:
    // that only stretches the 13px atlas and blurs above 100%.
    style.FontScaleDpi = scale;

    m_initialized = true;
    return true;
}

void ImGuiRenderer::Shutdown()
{
    if (!m_initialized)
        return;

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    m_initialized = false;
}

void ImGuiRenderer::NewFrame()
{
    // Start the Dear ImGui frame.
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiRenderer::Render()
{
    ImGui::Render();
    
    // Clear screen.
    const float clear_color[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    m_pd3dDeviceContext->OMSetRenderTargets(1, &m_mainRenderTargetView, nullptr);
    m_pd3dDeviceContext->ClearRenderTargetView(m_mainRenderTargetView, clear_color);
    
    // Render ImGui.
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    
    // Present, with vsync.
    m_pSwapChain->Present(1, 0);
}

void ImGuiRenderer::OnResize(const int width, const int height)
{
    if (m_pd3dDevice == nullptr)
        return;

    // Drop every reference to the back buffer before resizing it. CleanupRenderTarget() releases
    // our render-target view, but the immediate context still has it bound to the output-merger
    // stage from the previous frame - an indirect reference that keeps back buffer 0 alive. Unbind
    // it so no reference outlives the resize (required by the flip model, harmless here).
    CleanupRenderTarget();
    m_pd3dDeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    m_pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    CreateRenderTarget();
}

void ImGuiRenderer::OnDpiChanged(const UINT newDpi)
{
    if (!m_initialized)
        return;

    // Calculate new scale. This is the one place that must *not* go through App::GetDpiScale():
    // WM_DPICHANGED carries the authoritative new DPI in its wParam, so use that rather than
    // re-querying the window.
    const float newScale = newDpi / 96.0f;

    // Get current style.
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Reset style to default, then reapply custom style, then scale.
    // This ensures clean scaling without accumulation.
    style = ImGuiStyle();  // Reset to default.
    ApplyImGuiStyle();     // Reapply custom colors/settings.
    style.ScaleAllSizes(newScale);  // Scale style metrics.

    // Re-rasterize the font at the new DPI (see Initialize for why FontScaleDpi, not
    // io.FontGlobalScale). The reset above cleared it back to 1.0, so set it again.
    style.FontScaleDpi = newScale;
}

bool ImGuiRenderer::CreateDeviceD3D(const HWND hwnd)
{
    // Setup swap chain.
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    // Windowed-only app (custom DWM frame, never exclusive fullscreen), so no mode-switch
    // flag: nothing here ever changes the display mode, and leaving Flags at 0 keeps the
    // creation flags consistent with the ResizeBuffers(..., 0) call in OnResize().
    sd.Flags = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    // Blt model (bitblt), deliberately not the flip model. This window uses a custom frame
    // (WM_NCCALCSIZE makes the client area nearly the whole window), so the client back buffer is
    // a few pixels smaller than the outer window. Under the flip model DWM composites the swap
    // chain buffer directly and bilinearly stretches it whenever its size does not exactly match
    // the client region, which softened all text in windowed sizes (and only there - a maximized
    // window's client is set to the exact work-area rect, so it matched and stayed sharp). The blt
    // model presents through DWM's redirection surface, which composites 1:1 and keeps text crisp
    // at every window size. The app clears and fully redraws every frame, so DISCARD (which does
    // not preserve back-buffer contents between presents) is fine.
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // _DEBUG

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
        featureLevelArray, 2, D3D11_SDK_VERSION, &sd,
        &m_pSwapChain, &m_pd3dDevice, &featureLevel, &m_pd3dDeviceContext);
    
    if (hr != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void ImGuiRenderer::CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (m_pSwapChain) { m_pSwapChain->Release(); m_pSwapChain = nullptr; }
    if (m_pd3dDeviceContext) { m_pd3dDeviceContext->Release(); m_pd3dDeviceContext = nullptr; }
    if (m_pd3dDevice) { m_pd3dDevice->Release(); m_pd3dDevice = nullptr; }
}

void ImGuiRenderer::CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer = nullptr;
    m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer)
    {
        m_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_mainRenderTargetView);
        pBackBuffer->Release();
    }
}

void ImGuiRenderer::CleanupRenderTarget()
{
    if (m_mainRenderTargetView)
    {
        m_mainRenderTargetView->Release();
        m_mainRenderTargetView = nullptr;
    }
}
