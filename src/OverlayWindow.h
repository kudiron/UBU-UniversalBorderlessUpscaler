#pragma once

#include "Common.h"

class OverlayWindow
{
public:
    OverlayWindow();
    ~OverlayWindow();

    bool Create(HINSTANCE hInstance, const RECT& monitorRect);
    void Destroy();

    void Show(bool visible);
    bool IsVisible() const { return m_isVisible.load(); }

    HWND GetHwnd() const { return m_hwnd; }
    const RECT& GetMonitorRect() const { return m_monitorRect; }

    // Hedef pencere baska bir monitore tasindiginda overlay'i o monitore tasir.
    // Sadece main thread cagirir (Start/Stop ile ayni sirada).
    void SetMonitorRect(const RECT& monitorRect) { m_monitorRect = monitorRect; }

    // WndProc (main thread) ve RenderLoop (render thread) ayni anda erisebilir.
    void SetTargetWindow(HWND target) { m_targetHwnd.store(target); }
    HWND GetTargetWindow() const { return m_targetHwnd.load(); }

    void ProcessMessages();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND      m_hwnd      = nullptr;
    HINSTANCE m_hInstance = nullptr;
    RECT      m_monitorRect = {};
    std::atomic<bool> m_isVisible{ false };

    std::atomic<HWND> m_targetHwnd{ nullptr };
};