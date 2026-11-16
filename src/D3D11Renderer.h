#pragma once

#include "Common.h"

class D3D11Renderer
{
public:
    D3D11Renderer();
    ~D3D11Renderer();

    bool Initialize(HWND overlayHwnd, int width, int height);
    void Cleanup();

    bool Resize(int width, int height);
    bool RenderFrame(ID3D11Texture2D* capturedTexture, int srcWidth, int srcHeight,
                     const float uvMin[2], const float uvMax[2],
                     UpscaleMode mode, float sharpness);

    ID3D11Device* GetDevice() const { return m_device; }
    ID3D11DeviceContext* GetContext() const { return m_context; }
    bool IsDeviceLost() const { return m_deviceLost; }
    bool IsAllowTearing() const { return m_allowTearing; }

    // Cihaz + swapchain + shader'lar hazir mi? Start/Resize oncesi kontrol icin.
    bool IsReady() const { return m_device != nullptr && m_swapChain != nullptr && m_vs != nullptr; }

    bool RecoverDevice(HWND overlayHwnd, int width, int height);

    // ---------------------------------------------------------------------
    // Sentetik fare imleci.
    // Hedef pencereye hic dokunmadigimiz icin isletim sistemi imleci
    // gizlenir ve imlec overlay uzerine burada cizilir. Boylece imlec,
    // olceklenmis goruntude nereye denk geliyorsa ONDA gorunur.
    // Sadece render thread'den cagrilmalidir.
    // ---------------------------------------------------------------------
    void SetCursor(bool visible, float x, float y)
    {
        m_cursorVisible = visible;
        m_cursorX = x;
        m_cursorY = y;
    }

    // Texture yeniden yaratildiginda SRV cache'ini gecersiz kilar.
    // Sadece render thread'den cagrilmalidir (RenderFrame ile ayni thread).
    void InvalidateTextureCache()
    {
        SafeRelease(m_currentSRV);
        m_lastBoundTexture = nullptr;
    }

private:
    bool CreateDeviceAndSwapChain(HWND overlayHwnd, int width, int height);
    bool CreateRenderTarget();
    void ReleaseRenderTarget();
    bool CreateShaders();
    bool CreateSamplers();
    bool CreateConstantBuffer();

    bool CreateCursorResources();
    void ReleaseCursorResources();
    void DrawCursor();

    // Imlec dokusu: gercek Windows sistem imleci (fallback: prosedurel ok).
    int m_cursorTexW = 16;
    int m_cursorTexH = 22;
    int m_cursorHotX = 2;
    int m_cursorHotY = 2;

    HWND m_hwnd = nullptr;
    int m_outputWidth = 0;
    int m_outputHeight = 0;

    ID3D11Device*           m_device = nullptr;
    ID3D11DeviceContext*    m_context = nullptr;
    IDXGISwapChain1*        m_swapChain = nullptr;
    IDXGISwapChain2*        m_swapChain2 = nullptr;
    ID3D11RenderTargetView* m_backBufferRTV = nullptr;

    HANDLE m_frameLatencyWaitableObject = nullptr;
    bool   m_allowTearing = false;

    // Swapchain'in olusturuldugu bayraklar. ResizeBuffers'a AYNEN geri verilir;
    // aksi halde (or. ALLOW_TEARING dusurulurse) yeniden boyutlandirma basarisiz olur.
    UINT   m_swapChainFlags = 0;

    ID3D11VertexShader*     m_vs = nullptr;
    ID3D11PixelShader*      m_psOff = nullptr;
    ID3D11PixelShader*      m_psCAS = nullptr;
    ID3D11PixelShader*      m_psFSR = nullptr;
    ID3D11PixelShader*      m_psInteger = nullptr;

    ID3D11SamplerState*     m_linearSampler = nullptr;
    ID3D11SamplerState*     m_pointSampler = nullptr;
    ID3D11Buffer*           m_constantBuffer = nullptr;

    ID3D11ShaderResourceView* m_currentSRV = nullptr;
    ID3D11Texture2D*          m_lastBoundTexture = nullptr;

    // --- Sentetik imlec kaynaklari ---
    bool  m_cursorVisible = false;
    float m_cursorX = 0.0f;
    float m_cursorY = 0.0f;

    ID3D11Texture2D*    m_cursorTexture = nullptr;
    ID3D11ShaderResourceView* m_cursorSRV = nullptr;
    ID3D11Buffer*       m_cursorVB = nullptr;
    ID3D11VertexShader* m_cursorVS = nullptr;
    ID3D11PixelShader*  m_cursorPS = nullptr;
    ID3D11InputLayout*  m_cursorInputLayout = nullptr;
    ID3D11BlendState*   m_alphaBlend = nullptr;

    std::atomic<bool> m_deviceLost{ false };
};