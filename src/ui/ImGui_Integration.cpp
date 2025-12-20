// Copyright (c) 2025 Max Godman

#include "framework.h"
#include "ImGui_Integration.h"
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

    // Apply DPI scaling after style, so it scales everything.
    const UINT dpi = GetDpiForWindow(hwnd);
    float scale = dpi / 96.0f;  // 96 DPI is 100% scaling.
    ImGui::GetStyle().ScaleAllSizes(scale);
    io.FontGlobalScale = scale;

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

    CleanupRenderTarget();
    m_pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    CreateRenderTarget();
}

void ImGuiRenderer::OnDpiChanged(const UINT newDpi)
{
    if (!m_initialized)
        return;
    
    // Calculate new scale.
    const float newScale = newDpi / 96.0f;
    
    // Get current style.
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Reset style to default, then reapply custom style, then scale.
    // This ensures clean scaling without accumulation.
    style = ImGuiStyle();  // Reset to default.
    ApplyImGuiStyle();     // Reapply custom colors/settings.
    style.ScaleAllSizes(newScale);  // Scale everything.
    
    // Update font scale.
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = newScale;
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
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
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
