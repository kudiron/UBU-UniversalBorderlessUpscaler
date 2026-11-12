#pragma once

#include "Common.h"
#include "D3D11Renderer.h"
#include "CaptureManager.h"
#include "OverlayWindow.h"
#include <thread>

class UpscalerApp
{
public:
    UpscalerApp();
    ~UpscalerApp();

    bool Initialize(HINSTANCE hInstance);
    void Run();
    void Shutdown();

    void ToggleUpscaling();
    void SetMode(UpscaleMode mode);
    void SetSharpness(float sharpness);

    bool IsActive() const { return m_isActive.load(); }
    UpscaleMode GetMode() const { return m_currentMode.load(); }

private:
    bool RegisterHotkeys();
    void UnregisterHotkeys();

    bool InitTrayIcon();
    void RemoveTrayIcon();
    void ShowTrayMenu();
    void ShowShortcutsInfo();

    // Basit iki dilli metin tablosu (0 = Turkce, 1 = English).
    const wchar_t* Tr(const wchar_t* turkish, const wchar_t* english) const
    {
        return m_english.load() ? english : turkish;
    }

    bool StartUpscaling(HWND targetHwnd);
    void StopUpscaling();

    void RenderLoop();
    void PlayFeedbackSound(int soundType);

    static LRESULT CALLBACK HiddenWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleHiddenMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HINSTANCE m_hInstance = nullptr;
    HWND      m_hiddenHwnd = nullptr;
    NOTIFYICONDATAW m_nid = {};

    D3D11Renderer  m_renderer;
    CaptureManager m_captureMgr;
    OverlayWindow  m_overlayWindow;

    // --- Paylasilan durum: atomik ---
    // Bu uyeler main thread'den yazilip render thread'den okunuyor.
    std::atomic<HWND>        m_targetHwnd{ nullptr };
    std::atomic<UpscaleMode> m_currentMode{ UpscaleMode::CAS };
    std::atomic<float>       m_sharpness{ 0.75f };
    std::atomic<bool>        m_windowTransformed{ false };
    std::atomic<bool>        m_english{ false };      // Arayuz dili: false = TR, true = EN
    std::atomic<bool>        m_forceRedraw{ false };  // Mod/dil degisince kare yeniden cizilsin

    // Bu uyeler yalnizca m_toggleMutex altinda yazilir; main thread de bunlari
    // sadece ayni mutex altinda okur, dolayisiyla atomik olmalari gerekmez.
    LONG  m_origStyle   = 0;
    LONG  m_origExStyle = 0;
    RECT  m_origWindowRect = {};

    std::atomic<bool> m_isRunning{ false };
    std::atomic<bool> m_isActive{ false };

    // --- Thread-safety & debounce ---
    std::mutex   m_toggleMutex;        // Start/Stop cagrilarini siraya koyar
    std::mutex   m_renderMutex;        // D3D11Renderer + swapchain erisimini korur
    ULONGLONG    m_lastToggleTime = 0; // 300 ms debounce

    // Texture yeniden yaratildiginda renderer'in SRV cache'ini gecersiz kilmak icin
    uint32_t m_lastTextureGeneration = 0;

    std::thread m_renderThread;
};