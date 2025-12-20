// Copyright (c) 2025 Max Godman

// ImGui Integration for the application.

#pragma once

#include <d3d11.h>

struct ImGuiContext;

class ImGuiRenderer
{
public:
    ImGuiRenderer();
    ~ImGuiRenderer();

    /**
     * @brief Initialize ImGui with DirectX 11.
     */
    bool Initialize(const HWND hwnd);
    
    /**
     * @brief Shutdown and cleanup.
     */
    void Shutdown();
    
    // Frame management.
    void NewFrame();
    void Render();
    
    /**
     * @brief Check if initialized.
     */
    bool IsInitialized() const { return m_initialized; }
    
    /**
     * @brief Handle window resize.
     */
    void OnResize(const int width, const int height);
    
    /**
     * @brief Handle DPI change.
     */
    void OnDpiChanged(const UINT newDpi);

private:
    bool CreateDeviceD3D(const HWND hwnd);
    void CleanupDeviceD3D();
    void CreateRenderTarget();
    void CleanupRenderTarget();

    // DirectX 11 objects.
    ID3D11Device* m_pd3dDevice = nullptr;
    ID3D11DeviceContext* m_pd3dDeviceContext = nullptr;
    IDXGISwapChain* m_pSwapChain = nullptr;
    ID3D11RenderTargetView* m_mainRenderTargetView = nullptr;
    
    bool m_initialized = false;
    HWND m_hwnd = nullptr;
};

// Global renderer instance.
extern ImGuiRenderer* g_ImGuiRenderer;

// UI Rendering functions.
void RenderMainUI();
void ApplyImGuiStyle();
