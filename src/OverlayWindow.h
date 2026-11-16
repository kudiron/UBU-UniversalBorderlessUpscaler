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

    // --- Sentetik (yazilimsal) fare imleci -----------------------------------
    // TEK MOD: isletim sistemi imleci overlay aktifken tamamen gizlenir ve
    // gorunur imlec UBU tarafindan overlay uzerine cizilir. (Eskiden "Sistem /
    // Normal" secenegi gercek imleci birakiyordu; kaldirildi.)
    //
    // Bu fonksiyon gorunur (logical) imlec konumunu gunceller ve gercek imleci
    // hedef pencerenin eslenmis noktasina park eder. Iki ayrı thread cagirabilir:
    //   * UI thread     : fare mesajlari (WM_MOUSEMOVE / tiklama / tekerlek)
    //   * Render thread : her kare ciziminden hemen once (RenderLoop)
    // Delta, daima "son gorulen fiziksel konum" referans alinarak hesaplandigi
    // icin cagri SIRAYA BAGLI DEGILDIR: hangi thread once calisirsa calissin
    // sonuc Ayni kalir (idempotent). Boylece UI thread bir turlu mesgul olsa
    // (or. ses geri bildirimi) bile imlec hareket etmeye devam eder.
    bool UpdateCursor();

    // UI thread fare girdisi geldiginde render thread'i aninda uyandirmak icin
    // kullanilan auto-reset event.
    void SetWakeEvent(HANDLE hEvent) { m_wakeEvent = hEvent; }

    // Gorunur (logical) imlec konumu. Render thread yazarken UI thread
    // (ForwardMouseToTarget) okudugu icin kilit altinda alinir.
    bool GetLogicalCursor(POINT& outPoint) const;

    // --- Sentetik imlec durumu (WndProc/render thread yazar, render thread okur) ---
    bool GetCursorActive() const { return m_cursorActive.load(); }
    int  GetCursorPixelX() const { return m_cursorPixelX.load(); }
    int  GetCursorPixelY() const { return m_cursorPixelY.load(); }

    void ProcessMessages();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // Overlay uzerindeki fare girdisini hedef pencerenin ISTEMCI koordinatina
    // olcekleyip iletir (renderer'in letterbox hesabiyla birebir ayni mantik).
    // Kaynak konum, mesajin lParam'i degil gorunur (logical) imlec konumudur;
    // boylece fiziksel park ile PostMessage her zaman AYNI noktayi hedefler.
    bool ForwardMouseToTarget(UINT msg, WPARAM wParam);

    // Overlay istemci koordinatini hedef pencerenin istemci (veya ekran)
    // koordinatina cevirir. Iki fonksiyon da D3D11Renderer::RenderFrame'deki
    // letterbox/pillarbox hesabiyla birebir ayni matematigi kullanir.
    bool MapClientToTargetClient(int clientX, int clientY, int& outX, int& outY) const;
    bool MapClientToTargetScreen(int clientX, int clientY, POINT& outScreen) const;

    // Gorunur (logical) imleci gunceller. Tek davranis vardir (yazilimsal imlec):
    // konum DELTA ile guncellenir ve gercek (fiziksel) imlec hedef pencerenin
    // eslenmis noktasina park edilir.
    void RestoreRealCursor();

    HWND      m_hwnd      = nullptr;
    HINSTANCE m_hInstance = nullptr;
    RECT      m_monitorRect = {};
    std::atomic<bool> m_isVisible{ false };

    std::atomic<HWND> m_targetHwnd{ nullptr };

    // --- Sentetik imlec durumu ---
    // m_logicalCursor : kullanicinin GORDUGU konum (overlay istemci koordinati).
    // m_parkedClient  : fiziksel imlecin EN SONGORULEN konumu (overlay istemci);
    //                   delta hesabinin referansi.
    // Fare hareketi DELTA olarak toplanir; cunku fiziksel imleci hedef pencerenin
    // icine park ettigimizden isletim sistemi konumu park noktasina gore bildirir.
    //
    // KRITIK: Bu alanlara UI thread (fare mesajlari) VE render thread (her kare
    // oncesi) ayni anda erisebilir. Delta idempotent oldugu (son fiziksel konuma
    // gore hesaplandigi) icin cagri sirasi sonucu degistirmez; yine de yarim
    // guncellemeleri onlemek icin kilit altinda tutulur.
    mutable std::mutex m_cursorMutex;
    POINT m_logicalCursor = { 0, 0 };
    POINT m_parkedClient  = { 0, 0 };
    bool  m_haveParked    = false;

    std::atomic<bool> m_cursorActive{ false };
    std::atomic<int>  m_cursorPixelX{ 0 };
    std::atomic<int>  m_cursorPixelY{ 0 };

    // Yalnizca UI thread erisir: SetCursorPos kaynakli sentetik WM_MOUSEMOVE
    // tekrarlarinin hedef pencereye spamlanmasini onler.
    POINT  m_lastForwardedPos = { -1, -1 };
    HANDLE m_wakeEvent = nullptr;

    // Overlay aktifken isletim sistemi imlecini gorunmez yapan bos imlec.
    HCURSOR m_blankCursor = nullptr;
};