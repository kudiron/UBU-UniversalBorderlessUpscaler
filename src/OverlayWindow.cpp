#include "OverlayWindow.h"

static const wchar_t* OVERLAY_CLASS_NAME = L"UBU_OverlayClass";

OverlayWindow::OverlayWindow() {}
OverlayWindow::~OverlayWindow() { Destroy(); }

bool OverlayWindow::Create(HINSTANCE hInstance, const RECT& monitorRect)
{
    Destroy();

    m_hInstance = hInstance;
    m_monitorRect = monitorRect;

    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = OverlayWindow::WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = OVERLAY_CLASS_NAME;

    if (!RegisterClassExW(&wc))
    {
        DWORD err = GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS) return false;
    }

    int width  = monitorRect.right  - monitorRect.left;
    int height = monitorRect.bottom - monitorRect.top;

    // ---------------------------------------------------------------------
    // WS_EX_NOREDIRECTIONBITMAP (Intel UHD / DWM optimizasyonu - KRITIK):
    // Bu bayrak olmadan DWM, dolayli sunum icin pencerenin arkasina ekstra
    // bir "redirection bitmap" (tam ekran boyutunda bir yuzey) olusturur,
    // her kareyi oraya kopyalayip kompozit eder. Bu ekstra tam ekran kopya
    // Intel UHD gibi paylasimli bellekte calisan iGPU'larda olculebilir
    // sekilde ~5 FPS kayba yol acar.
    // Bu bayrakla pencere, bitmask'i olmayan saf bir DirectX sunum yuzeyi
    // olur: DWM ekstra doku olusturmaz, kare dogrudan tarayiciya gider.
    // (Zorunlu kosul: pencereye DXGI flip-model swapchain bagli olmali ->
    //  DXGI_SWAP_EFFECT_FLIP_DISCARD kullaniyoruz. Saglanmistir.)
    //
    // WS_EX_LAYERED | WS_EX_TRANSPARENT (TIKLAMA GECIRGENLIGI - KRITIK):
    // Overlay penceresi oyunun UZERINDE durur. Tiklama gecirgen OLMADIGI
    // surece tum gercek fare girdisini yutar; PostMessage ile gonderilen
    // sentetik mesajlari ise modern oyunlar (Raw Input / GetCursorPos /
    // GetAsyncKeyState kullandiklari icin) gormez -> hem fare kaymasi hem
    // tepkisizlik. WS_EX_LAYERED + WS_EX_TRANSPARENT kombinasyonu ile DWM
    // fare vurus-testini bu pencerede atlar ve olaylar ALTTAKI oyun
    // penceresine NATIVE olarak gider (dogru koordinat, dogru zamanlama).
    // (WS_EX_TRANSPARENT tek basina yalnizca ayni thread pencereleri icin
    //  gecerlidir; surec-disi gecirgenlik icin WS_EX_LAYERED sarttir.)
    // ---------------------------------------------------------------------
    m_hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_NOREDIRECTIONBITMAP |
        WS_EX_LAYERED  | WS_EX_TRANSPARENT,
        OVERLAY_CLASS_NAME,
        L"Universal Borderless Upscaler Overlay",
        WS_POPUP,
        monitorRect.left, monitorRect.top, width, height,
        nullptr, nullptr, hInstance, nullptr
    );
    if (!m_hwnd) return false;

    // Katmanli pencerede vurus-testinin atlanmasi icin alfa atamasi zorunludur.
    // 255 = tam opak; yine de WS_EX_TRANSPARENT sayesinde fare gecirgen kalir.
    SetLayeredWindowAttributes(m_hwnd, 0, 255, LWA_ALPHA);

    SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

    DWM_WINDOW_CORNER_PREFERENCE cornerPref = DWMWCP_DONOTROUND;
    DwmSetWindowAttribute(m_hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPref, sizeof(cornerPref));

    return true;
}

void OverlayWindow::Destroy()
{
    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
    m_isVisible = false;
    m_targetHwnd.store(nullptr);
}

void OverlayWindow::Show(bool visible)
{
    if (!m_hwnd) return;

    if (visible)
    {
        if (SetWindowPos(m_hwnd, HWND_TOPMOST,
                m_monitorRect.left, m_monitorRect.top,
                m_monitorRect.right  - m_monitorRect.left,
                m_monitorRect.bottom - m_monitorRect.top,
                SWP_NOACTIVATE | SWP_SHOWWINDOW))
        {
            m_isVisible = true;
        }
    }
    else
    {
        ShowWindow(m_hwnd, SW_HIDE);
        m_isVisible = false;
    }
}

void OverlayWindow::ProcessMessages()
{
    MSG msg;
    while (PeekMessageW(&msg, m_hwnd, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

LRESULT CALLBACK OverlayWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE)
    {
        CREATESTRUCTW* cs = (CREATESTRUCTW*)lParam;
        OverlayWindow* self = (OverlayWindow*)cs->lpCreateParams;
        if (self) SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)self);
    }

    OverlayWindow* self = (OverlayWindow*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (self)
        return self->HandleMessage(hwnd, msg, wParam, lParam);

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT OverlayWindow::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    // Fare mesajlari buraya artik ULASMAZ: pencere WS_EX_TRANSPARENT
    // oldugu icin DWM vurus-testini atlar; girdi native olarak alttaki
    // oyun penceresine gider. Sentetik yonlendirme kaldirildi.

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}