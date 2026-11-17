#include "OverlayWindow.h"

#include <cstring>

static const wchar_t* OVERLAY_CLASS_NAME = L"UBU_OverlayClass";

// Isletim sistemi imlecini tamamen gorunmez yapan 32x32 "bos" imlec.
// Monokrom imlec cizimi:  screen = (screen AND andMask) XOR xorMask
// andMask = 1 (ekrani koru) ve xorMask = 0 (degistirme) => hicbir sey cizilmez.
static HCURSOR CreateBlankCursor(HINSTANCE hInstance)
{
    BYTE andMask[32 * 32 / 8];
    BYTE xorMask[32 * 32 / 8];
    memset(andMask, 0xFF, sizeof(andMask));
    memset(xorMask, 0x00, sizeof(xorMask));
    return CreateCursor(hInstance, 0, 0, 32, 32, andMask, xorMask);
}

OverlayWindow::OverlayWindow() {}
OverlayWindow::~OverlayWindow() { Destroy(); }

bool OverlayWindow::Create(HINSTANCE hInstance, const RECT& monitorRect)
{
    Destroy();

    m_hInstance = hInstance;
    m_monitorRect = monitorRect;

    if (!m_blankCursor)
        m_blankCursor = CreateBlankCursor(hInstance);

    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    // CS_DBLCLKS: cift tiklamalarin da (WM_LBUTTONDBLCLK) pencerimize ulasmasini
    // saglar. Bu bayrak olmadan Windows cift tiklamayi iki ayri tek tiklamaya
    // indirger ve oyundaki cift-tiklama etkilesimleri bozulur.
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc   = OverlayWindow::WndProc;
    wc.hInstance     = hInstance;
    // Sinif imleci de BOS imlec: aktifken gercek imlec gizlenir ve yerine
    // D3D11Renderer tarafindan sentetik (yazilimsal) imlec cizilir.
    wc.hCursor       = m_blankCursor ? m_blankCursor : LoadCursor(nullptr, IDC_ARROW);
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
    // bir redirection bitmap (tam ekran boyutunda bir yuzey) olusturur, her
    // kareyi oraya kopyalayip kompozit eder. Bu ekstra tam ekran kopya Intel
    // UHD gibi paylasimli bellekte calisan iGPU'larda ~5 FPS kayba yol acar.
    // Bu bayrakla pencere, bitmask'i olmayan saf bir DirectX sunum yuzeyi
    // olur. (Zorunlu kosul: pencereye DXGI flip-model swapchain bagli olmali
    //  -> DXGI_SWAP_EFFECT_FLIP_DISCARD kullaniyoruz. Saglanmistir.)
    //
    // FARE GIRDISI (KRITIK):
    // Overlay tam monitoru kaplar ve hedef pencerenin olceklenmis karesini
    // gosterir. Hedef pencere DUSUK cozunurlukte kaldigi icin (FPS kazanci
    // korunur) gercek fare girdisini BURADA yakalariz. Bu yuzden pencere
    // TIKLAMA GECIRGEN DEGILDIR: WS_EX_LAYERED ve WS_EX_TRANSPARENT
    // KALDIRILDI, cunku DWM fare vurus-testini atlayinca olaylar penceremize
    // hic ulasmiyordu.
    // WS_EX_NOACTIVATE: overlay asla odak calmaz; oyun on planda kalir ve
    // klavye girdisi dogrudan oyuna gider.
    // ---------------------------------------------------------------------
    m_hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_NOREDIRECTIONBITMAP,
        OVERLAY_CLASS_NAME,
        L"Universal Borderless Upscaler Overlay",
        WS_POPUP,
        monitorRect.left, monitorRect.top, width, height,
        nullptr, nullptr, hInstance, nullptr
    );
    if (!m_hwnd) return false;

    SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

    DWM_WINDOW_CORNER_PREFERENCE cornerPref = DWMWCP_DONOTROUND;
    DwmSetWindowAttribute(m_hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPref, sizeof(cornerPref));

    return true;
}

void OverlayWindow::Destroy()
{
    if (m_hwnd)
    {
        ReleaseCapture();
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
    m_isVisible = false;
    m_targetHwnd.store(nullptr);

    if (m_blankCursor)
    {
        DestroyCursor(m_blankCursor);
        m_blankCursor = nullptr;
    }

    m_cursorActive.store(false);
    {
        std::lock_guard<std::mutex> lock(m_cursorMutex);
        m_logicalCursor = { 0, 0 };
        m_parkedClient  = { 0, 0 };
        m_haveParked    = false;
    }
    m_lastForwardedPos = { -1, -1 };
    m_lastForwardedTargetClientX = -1;
    m_lastForwardedTargetClientY = -1;
    m_cachedTargetHwnd = nullptr;
    m_cachedTargetClientRect = {};
    m_cachedClientTick = 0;
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
            // Yeni gosterim: imlec esleme referansini sifirla ve gorunur
            // konumu farenin gercek konumundan hemen baslat.
            {
                std::lock_guard<std::mutex> lock(m_cursorMutex);
                m_haveParked = false;
            }
            m_lastForwardedPos = { -1, -1 };
            UpdateCursor();
        }
    }
    else
    {
        ShowWindow(m_hwnd, SW_HIDE);
        m_isVisible = false;
        m_cursorActive.store(false);
        RestoreRealCursor();
    }
}

bool OverlayWindow::GetLogicalCursor(POINT& outPoint) const
{
    std::lock_guard<std::mutex> lock(m_cursorMutex);
    if (!m_cursorActive.load()) return false;
    outPoint = m_logicalCursor;
    return true;
}

// Park edilmis gercek imleci, kullanicinin gordugu (logical) konuma geri tasir.
// Overlay kapanirken cagrilir; boylece kullanici imleci kaybetmis gibi olmaz.
void OverlayWindow::RestoreRealCursor()
{
    std::lock_guard<std::mutex> lock(m_cursorMutex);
    if (m_haveParked)
    {
        POINT p = { m_monitorRect.left + m_logicalCursor.x,
                    m_monitorRect.top  + m_logicalCursor.y };
        SetCursorPos(p.x, p.y);
    }
    m_haveParked = false;
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
    // -----------------------------------------------------------------------
    // KRITIK / FARE:
    // 1) Gorunur (logical) imleci DELTA ile guncelleriz ve sentetik imleci
    //    renderer overlay'e cizer.
    // 2) Gercek (fiziksel) imleci hedef pencerenin eslenmis noktasina park
    //    ederiz. Bircok oyun fare konumunu kendi lParam'indan degil
    //    GetCursorPos() ile okur; bu park sayesinde oyun DOGRU noktayi gorur
    //    (butonlar tam gorundukleri yerde tutar).
    // -----------------------------------------------------------------------
    case WM_MOUSEMOVE:
        if (UpdateCursor() && m_wakeEvent)
            SetEvent(m_wakeEvent);
        ForwardMouseToTarget(msg, wParam);
        return 0;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_XBUTTONDOWN:
        if (UpdateCursor() && m_wakeEvent)
            SetEvent(m_wakeEvent);
        // Surukleme sirasinda imlec monitor disina ciksa bile olaylari al.
        SetCapture(hwnd);
        ForwardMouseToTarget(msg, wParam);
        return 0;

    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_XBUTTONUP:
        if (UpdateCursor() && m_wakeEvent)
            SetEvent(m_wakeEvent);
        ForwardMouseToTarget(msg, wParam);
        if ((wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON | MK_XBUTTON1 | MK_XBUTTON2)) == 0)
            ReleaseCapture();
        return 0;

    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
        if (UpdateCursor() && m_wakeEvent)
            SetEvent(m_wakeEvent);
        ForwardMouseToTarget(msg, wParam);
        return 0;

    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
        // Tekerlek mesajinda lParam EKRAN koordinatidir; kaynak konum olarak
        // gorunur (logical) imleci kullandigimiz icin once onu canli konumla
        // senkronlar, sonra iletiriz.
        if (UpdateCursor() && m_wakeEvent)
            SetEvent(m_wakeEvent);
        ForwardMouseToTarget(msg, wParam);
        return 0;

    case WM_MOUSEACTIVATE:
        // Overlay asla aktiflesmesin: odak ve klavye oyunda kalsin.
        return MA_NOACTIVATE;

    case WM_SETCURSOR:
    {
        // YAZILIMSAL IMLEÇ (TEK MOD): Overlay aktifken isletim sistemi imleci
        // tamamen gizlenir; gorunur imlec UBU tarafindan overlay'e cizilir.
        if (m_isVisible.load() && m_blankCursor)
            SetCursor(m_blankCursor);
        else
            SetCursor(LoadCursor(nullptr, IDC_ARROW));
        return TRUE;
    }

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

// -----------------------------------------------------------------------------
// Gorunur (logical) imleci gunceller ve gercek imleci hedefe park eder.
//
// Delta, mesajin lParam'indan degil, GetCursorPos() ile alinan CANLI konumdan
// hesaplanir. Sebep: imleci her harekette park ettigimiz icin, park
// cagrisindan ONCE uretilmis "bayat" bir WM_MOUSEMOVE'in lParam'i, park
// sonrasi konumla karistirildiginda imlec buyuk bir sicrama yapiyordu
// (kullanicinin bildirdigi "fare hassasiyeti" hatasi). GetCursorPos her zaman
// guncel konumu dondurur; dolayisiyla hangi sirada gelirse gelsin delta hem
// DOGRU hem de kayipsiz hesaplanir ve imlec 1:1 (isletim sistemi imleci ile
// birebir ayni hizda) hareket eder.
// -----------------------------------------------------------------------------
bool OverlayWindow::UpdateCursor()
{
    HWND target = m_targetHwnd.load();
    if (!target || !IsWindow(target)) return false;

    const int outW = m_monitorRect.right  - m_monitorRect.left;
    const int outH = m_monitorRect.bottom - m_monitorRect.top;
    if (outW <= 0 || outH <= 0) return false;

    POINT actual = {};
    if (!GetCursorPos(&actual)) return false;

    // Gercek imlecin overlay istemcisindeki CANLI konumu.
    const int curX = actual.x - m_monitorRect.left;
    const int curY = actual.y - m_monitorRect.top;

    std::lock_guard<std::mutex> lock(m_cursorMutex);

    // -------------------------------------------------------------------------
    // YAZILIMSAL IMLEC MODU (TEK MOD): Gercek imlec gizlenir, park edilir ve
    // sentetik imlec cizilir. Oyun fareyi GetCursorPos() ile okuyorsa gereklidir.
    // -------------------------------------------------------------------------
    bool moved = false;
    if (!m_haveParked)
    {
        // Ilk hareket: gorunur konum, farenin gercek konumudur.
        m_logicalCursor.x = curX;
        m_logicalCursor.y = curY;
        m_haveParked = true;
        moved = true;
    }
    else
    {
        // Delta = park noktasina gore GERCEK konum farki (1:1).
        const int dx = curX - m_parkedClient.x;
        const int dy = curY - m_parkedClient.y;
        if (dx != 0 || dy != 0)
        {
            m_logicalCursor.x += dx;
            m_logicalCursor.y += dy;
            moved = true;
        }
    }

    if (!moved) return false;

    if (m_logicalCursor.x < 0)        m_logicalCursor.x = 0;
    if (m_logicalCursor.y < 0)        m_logicalCursor.y = 0;
    if (m_logicalCursor.x > outW - 1) m_logicalCursor.x = outW - 1;
    if (m_logicalCursor.y > outH - 1) m_logicalCursor.y = outH - 1;

    m_cursorPixelX.store(m_logicalCursor.x);
    m_cursorPixelY.store(m_logicalCursor.y);
    m_cursorActive.store(true);

    POINT mapped = {};
    if (MapClientToTargetScreen(m_logicalCursor.x, m_logicalCursor.y, mapped))
    {
        if (actual.x != mapped.x || actual.y != mapped.y)
        {
            SetCursorPos(mapped.x, mapped.y);

            // Park referansini sistemin GERCEK konumundan al: SetCursorPos ekran
            // kenarinda kirpilabilir; bu durumda deltayi yanlis hesaplamamak icin
            // gercek degeri kullaniriz.
            POINT after = {};
            if (GetCursorPos(&after))
            {
                m_parkedClient.x = after.x - m_monitorRect.left;
                m_parkedClient.y = after.y - m_monitorRect.top;
            }
            else
            {
                m_parkedClient.x = mapped.x - m_monitorRect.left;
                m_parkedClient.y = mapped.y - m_monitorRect.top;
            }
        }
        else
        {
            m_parkedClient.x = curX;
            m_parkedClient.y = curY;
        }
    }
    else
    {
        // Esleme yapilamadi (hedef kucultulmus vb.): gercek imleci oldugu
        // yerde birak, park referansini CANLI konuma esitle.
        m_parkedClient.x = curX;
        m_parkedClient.y = curY;
    }

    return true;
}

// -----------------------------------------------------------------------------
// Koordinat eslemesi: overlay istemcisi -> hedef istemcisi / ekran.
// D3D11Renderer::RenderFrame ile BIREBIR ayni letterbox/pillarbox matematigi.
// -----------------------------------------------------------------------------
bool OverlayWindow::MapClientToTargetClient(int clientX, int clientY, int& outX, int& outY) const
{
    HWND target = m_targetHwnd.load();
    if (!target || target == m_hwnd || !IsWindow(target)) return false;

    RECT rcClient = {};
    ULONGLONG now = GetTickCount64();
    // Önbellekleme (Caching): Her WM_MOUSEMOVE olayında GetClientRect çağrısını
    // tekrar tekrar yapmamak için önbellekte tutuyoruz (pencere aynıysa ve 50ms geçmemişse veya hızlı kontrol).
    // Ayrıca tıklama anında veya boyut değişiminde güncellenir.
    if (m_cachedTargetHwnd == target && (now - m_cachedClientTick < 50))
    {
        rcClient = m_cachedTargetClientRect;
    }
    else
    {
        if (!GetClientRect(target, &rcClient)) return false;
        m_cachedTargetHwnd = target;
        m_cachedTargetClientRect = rcClient;
        m_cachedClientTick = now;
    }
    const int srcW = rcClient.right - rcClient.left;
    const int srcH = rcClient.bottom - rcClient.top;
    if (srcW <= 0 || srcH <= 0) return false;

    const int outW = m_monitorRect.right  - m_monitorRect.left;
    const int outH = m_monitorRect.bottom - m_monitorRect.top;
    if (outW <= 0 || outH <= 0) return false;

    const float targetAspect = (float)srcW / (float)srcH;
    const float outputAspect = (float)outW / (float)outH;

    float drawW = (float)outW;
    float drawH = (float)outH;
    float offsetX = 0.0f;
    float offsetY = 0.0f;

    if (std::abs(targetAspect - outputAspect) > 0.005f)
    {
        if (targetAspect > outputAspect)
        {
            drawH = (float)outW / targetAspect;      // letterbox (ust/alt serit)
            offsetY = ((float)outH - drawH) * 0.5f;
        }
        else
        {
            drawW = (float)outH * targetAspect;      // pillarbox (sag/sol serit)
            offsetX = ((float)outW - drawW) * 0.5f;
        }
    }

    float u = ((float)clientX - offsetX) / std::max(1.0f, drawW);
    float v = ((float)clientY - offsetY) / std::max(1.0f, drawH);
    u = std::clamp(u, 0.0f, 0.9999f);
    v = std::clamp(v, 0.0f, 0.9999f);

    int cx = (int)(u * (float)srcW);
    int cy = (int)(v * (float)srcH);
    if (cx < 0) cx = 0;
    if (cy < 0) cy = 0;
    if (cx >= srcW) cx = srcW - 1;
    if (cy >= srcH) cy = srcH - 1;

    outX = cx;
    outY = cy;
    return true;
}

bool OverlayWindow::MapClientToTargetScreen(int clientX, int clientY, POINT& outScreen) const
{
    int cx = 0, cy = 0;
    if (!MapClientToTargetClient(clientX, clientY, cx, cy)) return false;

    POINT pt = { cx, cy };
    if (!ClientToScreen(m_targetHwnd.load(), &pt)) return false;

    outScreen = pt;
    return true;
}

// -----------------------------------------------------------------------------
// Fare mesajini hedef pencereye iletir. Kaynak konum, mesajin lParam'i DEGIL
// gorunur (logical) imlec konumudur; boylece fiziksel park (SetCursorPos) ile
// PostMessage her zaman AYNI hedef noktayi isaret eder ve iki okuma yolu da
// (GetCursorPos / mesaj koordinati) tutarli kalir. Tekerlek mesajlarinda ise
// lParam EKRAN koordinati olmasi gerektigi icin hedef EKRAN koordinati uretilir.
// -----------------------------------------------------------------------------
bool OverlayWindow::ForwardMouseToTarget(UINT msg, WPARAM wParam)
{
    HWND target = m_targetHwnd.load();
    if (!target || target == m_hwnd || !IsWindow(target)) return false;

    POINT logical = {};
    if (!GetLogicalCursor(logical)) return false;

    if (msg == WM_MOUSEMOVE)
    {
        if (logical.x == m_lastForwardedPos.x && logical.y == m_lastForwardedPos.y)
            return false;
        m_lastForwardedPos = logical;
    }

    int cx = 0, cy = 0;
    if (!MapClientToTargetClient(logical.x, logical.y, cx, cy))
        return false;

    LPARAM outParam;
    // 1. WM_MOUSEMOVE Coalescing / Tekrar Önleme:
    // Aynı hedef (targetClientX, targetClientY) koordinatına arka arkaya gereksiz
    // PostMessage(WM_MOUSEMOVE) göndermeyi engelle (yalnızca koordinat gerçekten değiştiyse mesaj gönder).
    if (msg == WM_MOUSEMOVE)
    {
        if (cx == m_lastForwardedTargetClientX && cy == m_lastForwardedTargetClientY)
            return false;
        m_lastForwardedTargetClientX = cx;
        m_lastForwardedTargetClientY = cy;
    }
    else if (msg >= WM_LBUTTONDOWN && msg <= WM_XBUTTONDBLCLK)
    {
        // Tıklama anında hedef client cache'ini tazele (önbellek güncellemeleri için)
        m_cachedClientTick = 0;
    }
    if (msg == WM_MOUSEWHEEL || msg == WM_MOUSEHWHEEL)
    {
        // KRITIK: Tekerlek mesajlarinda lParam ISTEMCI degil EKRAN koordinatidir
        // ("The coordinate is relative to the upper-left corner of the screen").
        // Oyun kendi ScreenToClient'ini uyguladigi icin, hesapladigimiz istemci
        // noktasini once hedef pencerenin EKRAN koordinatina ceviriyoruz.
        POINT tp = { cx, cy };
        if (!ClientToScreen(target, &tp)) return false;
        outParam = MAKELPARAM((WORD)(short)tp.x, (WORD)(short)tp.y);
    }
    else
    {
        outParam = MAKELPARAM((WORD)cx, (WORD)cy);
    }

    PostMessageW(target, msg, wParam, outParam);
    return true;
}
