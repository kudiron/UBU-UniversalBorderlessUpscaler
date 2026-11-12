#include "UpscalerApp.h"

#define WM_TRAYICON (WM_USER + 101)
#define ID_HOTKEY_TOGGLE   100
#define ID_HOTKEY_MODE_CAS 101
#define ID_HOTKEY_MODE_FSR 102
#define ID_HOTKEY_MODE_INT 103
#define ID_HOTKEY_MODE_OFF 104

#define ID_MENU_STATUS      2001
#define ID_MENU_TOGGLE      2002
#define ID_MENU_MODE_CAS    2003
#define ID_MENU_MODE_FSR    2004
#define ID_MENU_MODE_INT    2005
#define ID_MENU_ABOUT       2006
#define ID_MENU_EXIT        2007
#define ID_MENU_MODE_OFF    2008
#define ID_MENU_LANG_TR     2009
#define ID_MENU_LANG_EN     2010

static const wchar_t* HIDDEN_CLASS_NAME = L"UBU_HiddenHostClass";

// ---- Constructor / Destructor -----------------------------------------------

UpscalerApp::UpscalerApp()
{
}

UpscalerApp::~UpscalerApp()
{
    Shutdown();
}

// ---- Initialize / Run / Shutdown --------------------------------------------

bool UpscalerApp::Initialize(HINSTANCE hInstance)
{
    m_hInstance = hInstance;

    WNDCLASSEXW wc   = {};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc   = UpscalerApp::HiddenWndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = HIDDEN_CLASS_NAME;
    RegisterClassExW(&wc);

    m_hiddenHwnd = CreateWindowExW(
        WS_EX_TOOLWINDOW,          // Alt+Tab ve gorev cubugunda gorunmez
        HIDDEN_CLASS_NAME,
        L"UBU_HiddenHost",
        WS_POPUP,                  // Gizli, gosterilmeyen ust seviye pencere:
        0, 0, 0, 0,                // mesaj-only pencerelerin aksine
        nullptr,                   // SetForegroundWindow ve TrackPopupMenu
        nullptr,                   // duzgun calisir (menu disari tiklayinca kapanir).
        hInstance,
        this
    );
    if (!m_hiddenHwnd) return false;

    HMONITOR   hMon = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi  = { sizeof(MONITORINFO) };
    GetMonitorInfoW(hMon, &mi);

    int monW = mi.rcMonitor.right  - mi.rcMonitor.left;
    int monH = mi.rcMonitor.bottom - mi.rcMonitor.top;

    // Overlay + renderer burada kurulur; basarisiz olursa StartUpscaling
    // sirasinda yeniden denenir (uygulama yine de ayakta kalir).
    if (m_overlayWindow.Create(hInstance, mi.rcMonitor))
    {
        if (!m_renderer.Initialize(m_overlayWindow.GetHwnd(), monW, monH))
        {
            m_overlayWindow.Destroy();
        }
    }

    RegisterHotkeys();
    InitTrayIcon();

    m_isRunning = true;
    m_renderThread = std::thread(&UpscalerApp::RenderLoop, this);

    return true;
}

void UpscalerApp::Run()
{
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void UpscalerApp::Shutdown()
{
    m_isRunning = false;

    if (m_renderThread.joinable())
        m_renderThread.join();

    {
        std::lock_guard<std::mutex> lock(m_toggleMutex);
        if (m_isActive.exchange(false))
        {
            m_overlayWindow.SetTargetWindow(nullptr);
            m_overlayWindow.Show(false);
            m_captureMgr.StopCapture();

            HWND target = m_targetHwnd.load();
            if (m_windowTransformed.load() && target && IsWindow(target))
            {
                SetWindowLongPtrW(target, GWL_STYLE,   m_origStyle);
                SetWindowLongPtrW(target, GWL_EXSTYLE, m_origExStyle);
                SetWindowPos(target, HWND_NOTOPMOST,
                    m_origWindowRect.left, m_origWindowRect.top,
                    m_origWindowRect.right  - m_origWindowRect.left,
                    m_origWindowRect.bottom - m_origWindowRect.top,
                    SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            }
            m_windowTransformed.store(false);
            m_targetHwnd.store(nullptr);
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_renderMutex);
        m_renderer.Cleanup();
    }

    if (m_overlayWindow.GetHwnd())
    {
        m_overlayWindow.ProcessMessages();
        m_overlayWindow.Destroy();
    }

    UnregisterHotkeys();
    RemoveTrayIcon();

    if (m_hiddenHwnd)
    {
        DestroyWindow(m_hiddenHwnd);
        m_hiddenHwnd = nullptr;
    }
}

// ---- Hotkeys / Tray ---------------------------------------------------------

bool UpscalerApp::RegisterHotkeys()
{
    if (!m_hiddenHwnd) return false;
    bool ok = true;

    ok &= RegisterHotKey(m_hiddenHwnd, ID_HOTKEY_TOGGLE,
                         MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, 'Z') != FALSE;
    ok &= RegisterHotKey(m_hiddenHwnd, ID_HOTKEY_MODE_CAS,
                         MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, '1') != FALSE;
    ok &= RegisterHotKey(m_hiddenHwnd, ID_HOTKEY_MODE_FSR,
                         MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, '2') != FALSE;
    ok &= RegisterHotKey(m_hiddenHwnd, ID_HOTKEY_MODE_INT,
                         MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, '3') != FALSE;
    RegisterHotKey(m_hiddenHwnd, ID_HOTKEY_MODE_OFF,
                   MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, '0');

    if (!ok)
    {
        MessageBoxW(nullptr,
            L"UBU kisayol tuslarindan bazilari baska bir uygulama tarafindan\n"
            L"kullaniliyor. Lutfen o uygulamayi kapatip UBU'yu yeniden baslatin.",
            L"UBU - Kisayol Uyarisi",
            MB_OK | MB_ICONWARNING);
    }
    return ok;
}

void UpscalerApp::UnregisterHotkeys()
{
    if (!m_hiddenHwnd) return;
    UnregisterHotKey(m_hiddenHwnd, ID_HOTKEY_TOGGLE);
    UnregisterHotKey(m_hiddenHwnd, ID_HOTKEY_MODE_CAS);
    UnregisterHotKey(m_hiddenHwnd, ID_HOTKEY_MODE_FSR);
    UnregisterHotKey(m_hiddenHwnd, ID_HOTKEY_MODE_INT);
    UnregisterHotKey(m_hiddenHwnd, ID_HOTKEY_MODE_OFF);
}

bool UpscalerApp::InitTrayIcon()
{
    ZeroMemory(&m_nid, sizeof(m_nid));
    m_nid.cbSize          = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd            = m_hiddenHwnd;
    m_nid.uID             = 1;
    m_nid.uFlags          = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = WM_TRAYICON;
    m_nid.hIcon           = LoadIcon(nullptr, IDI_APPLICATION);
    wcscpy_s(m_nid.szTip, L"Universal Borderless Upscaler (UBU) v1.0\nAlt+Shift+Z: Fullscreen | Right-click: Menu");
    return Shell_NotifyIconW(NIM_ADD, &m_nid) != FALSE;
}

void UpscalerApp::RemoveTrayIcon()
{
    if (m_nid.cbSize)
    {
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
        ZeroMemory(&m_nid, sizeof(m_nid));
    }
}

void UpscalerApp::ShowTrayMenu()
{
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    const bool active = m_isActive.load();
    const bool en     = m_english.load();

    // --- Durum satiri (bilgilendirme, tiklanamaz) ---
    std::wstring statusStr;
    if (active)
        statusStr = en ? L"Status: ACTIVE (Borderless Fullscreen)"
                       : L"Durum: AKTIF (Kenarliksiz Tam Ekran)";
    else
        statusStr = en ? L"Status: IDLE" : L"Durum: BEKLEMEDE";
    AppendMenuW(hMenu, MF_STRING | MF_GRAYED, ID_MENU_STATUS, statusStr.c_str());
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

    // --- Ana eylem ---
    AppendMenuW(hMenu, MF_STRING, ID_MENU_TOGGLE,
        active ? (en ? L"Exit Fullscreen\tAlt+Shift+Z" : L"Tam Ekrandan Cik\tAlt+Shift+Z")
               : (en ? L"Borderless Fullscreen (Active Window)\tAlt+Shift+Z"
                     : L"Aktif Pencereyi Tam Ekran Yap\tAlt+Shift+Z"));
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

    // --- Keskinlestirme modu ---
    AppendMenuW(hMenu, MF_STRING | MF_GRAYED, 0,
        en ? L"Sharpening Mode" : L"Keskinlestirme Modu");

    const UpscaleMode mode = m_currentMode.load();
    UINT flagsOff = (mode == UpscaleMode::OFF)     ? (MF_STRING | MF_CHECKED) : MF_STRING;
    UINT flagsCas = (mode == UpscaleMode::CAS)     ? (MF_STRING | MF_CHECKED) : MF_STRING;
    UINT flagsFsr = (mode == UpscaleMode::FSR)     ? (MF_STRING | MF_CHECKED) : MF_STRING;
    UINT flagsInt = (mode == UpscaleMode::INTEGER) ? (MF_STRING | MF_CHECKED) : MF_STRING;

    AppendMenuW(hMenu, flagsCas, ID_MENU_MODE_CAS,
        en ? L"CAS - Light & Balanced (Recommended)\tAlt+Shift+1"
           : L"CAS - Hafif ve Dengeli (Onerilen)\tAlt+Shift+1");
    AppendMenuW(hMenu, flagsFsr, ID_MENU_MODE_FSR,
        en ? L"FSR 1.0 - Maximum Edge Sharpness\tAlt+Shift+2"
           : L"FSR 1.0 - Maksimum Kenar Netligi\tAlt+Shift+2");
    AppendMenuW(hMenu, flagsInt, ID_MENU_MODE_INT,
        en ? L"Integer - Crisp Pixels (Retro/2D)\tAlt+Shift+3"
           : L"Integer - Keskin Piksel (Retro/2D)\tAlt+Shift+3");
    AppendMenuW(hMenu, flagsOff, ID_MENU_MODE_OFF,
        en ? L"Off - Hardware Bilinear (Fastest)\tAlt+Shift+0"
           : L"Kapali - Donanim Bilinear (En Hizli)\tAlt+Shift+0");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

    // --- Dil secimi ---
    AppendMenuW(hMenu, MF_STRING | MF_GRAYED, 0, en ? L"Language" : L"Dil / Language");
    AppendMenuW(hMenu, (en ? MF_STRING : (MF_STRING | MF_CHECKED)), ID_MENU_LANG_TR, L"Turkce");
    AppendMenuW(hMenu, (en ? (MF_STRING | MF_CHECKED) : MF_STRING), ID_MENU_LANG_EN, L"English");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

    AppendMenuW(hMenu, MF_STRING, ID_MENU_ABOUT,
        en ? L"Shortcuts & About..." : L"Kisayollar ve Hakkinda...");
    AppendMenuW(hMenu, MF_STRING, ID_MENU_EXIT,
        en ? L"Exit (Safe Quit)" : L"Cikis (Guvenli Kapatma)");

    SetForegroundWindow(m_hiddenHwnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_LEFTALIGN, pt.x, pt.y, 0, m_hiddenHwnd, nullptr);
    DestroyMenu(hMenu);
}

void UpscalerApp::ShowShortcutsInfo()
{
    const bool en = m_english.load();

    std::wstring info = en
        ? L"Universal Borderless Upscaler (UBU) v1.0\n\n"
          L"Shortcuts:\n"
          L"  Alt + Shift + Z : Borderless Fullscreen On / Off\n"
          L"  Alt + Shift + 1 : CAS (Light & Balanced - Recommended)\n"
          L"  Alt + Shift + 2 : FSR 1.0 (EASU + RCAS)\n"
          L"  Alt + Shift + 3 : Integer Scaling (Retro / 2D)\n"
          L"  Alt + Shift + 0 : Sharpening Off (Hardware Bilinear)\n\n"
        : L"Universal Borderless Upscaler (UBU) v1.0\n\n"
          L"Kisayollar:\n"
          L"  Alt + Shift + Z : Kenarliksiz Tam Ekran Ac / Kapat\n"
          L"  Alt + Shift + 1 : CAS (Hafif ve Dengeli - Onerilen)\n"
          L"  Alt + Shift + 2 : FSR 1.0 (EASU + RCAS)\n"
          L"  Alt + Shift + 3 : Integer Olcekleme (Retro / 2D)\n"
          L"  Alt + Shift + 0 : Keskinlestirme Kapali (Donanim Bilinear)\n\n";

    info += en ? L"Current mode: " : L"Gecerli mod: ";
    info += GetUpscaleModeName(m_currentMode.load());
    info += en ? L"\nStatus: " : L"\nDurum: ";
    info += m_isActive.load()
        ? (en ? L"ACTIVE (Borderless Fullscreen)" : L"AKTIF (Kenarliksiz Tam Ekran)")
        : (en ? L"IDLE" : L"BEKLEMEDE");

    MessageBoxW(nullptr, info.c_str(),
        en ? L"UBU - Information" : L"UBU - Bilgilendirme",
        MB_OK | MB_ICONINFORMATION);
}

// Senkron ses: kisa Beep'ler UI thread'ini ~250 ms bloke eder, detached thread UB'sini onler.
void UpscalerApp::PlayFeedbackSound(int soundType)
{
    switch (soundType)
    {
    case 1: Beep(880, 70);  Beep(1320, 90); break;
    case 2: Beep(1320, 70); Beep(880, 90);  break;
    case 3: Beep(1000, 70); break;
    case 4: Beep(1200, 60); Sleep(30); Beep(1200, 60); break;
    case 5: Beep(1400, 50); Sleep(25); Beep(1400, 50); Sleep(25); Beep(1400, 50); break;
    }
}

// ---- Toggle / Start / Stop --------------------------------------------------

// Masaustu, gorev cubugu ve kabuk pencereleri tam ekrana alinmamali.
static bool IsShellWindow(HWND hwnd)
{
    if (!hwnd) return true;
    if (hwnd == GetShellWindow() || hwnd == GetDesktopWindow()) return true;

    wchar_t cls[64] = {};
    if (GetClassNameW(hwnd, cls, ARRAYSIZE(cls)) == 0) return false;

    return (wcscmp(cls, L"Shell_TrayWnd") == 0)
        || (wcscmp(cls, L"Shell_SecondaryTrayWnd") == 0)
        || (wcscmp(cls, L"Progman") == 0)
        || (wcscmp(cls, L"WorkerW") == 0);
}

void UpscalerApp::ToggleUpscaling()
{
    ULONGLONG now = GetTickCount64();
    if (now - m_lastToggleTime < 300ULL) return;
    m_lastToggleTime = now;

    if (!m_toggleMutex.try_lock()) return;
    std::lock_guard<std::mutex> lock(m_toggleMutex, std::adopt_lock);

    if (m_isActive.load())
    {
        StopUpscaling();
        PlayFeedbackSound(2);
    }
    else
    {
        HWND targetHwnd = GetForegroundWindow();
        if (targetHwnd
            && targetHwnd != m_overlayWindow.GetHwnd()
            && targetHwnd != m_hiddenHwnd
            && !IsShellWindow(targetHwnd))
        {
            if (StartUpscaling(targetHwnd))
                PlayFeedbackSound(1);
        }
    }
}

void UpscalerApp::SetMode(UpscaleMode mode)
{
    m_currentMode.store(mode);
    m_forceRedraw.store(true);  // Yeni kare gelmese bile ekran hemen guncellensin

    switch (mode)
    {
    case UpscaleMode::OFF:     PlayFeedbackSound(3); break;
    case UpscaleMode::CAS:     PlayFeedbackSound(3); break;
    case UpscaleMode::FSR:     PlayFeedbackSound(4); break;
    case UpscaleMode::INTEGER: PlayFeedbackSound(5); break;
    }
}

void UpscalerApp::SetSharpness(float sharpness)
{
    m_sharpness.store(std::clamp(sharpness, 0.0f, 1.0f));
    m_forceRedraw.store(true);
}

bool UpscalerApp::StartUpscaling(HWND targetHwnd)
{
    if (!IsWindow(targetHwnd)) return false;

    // Kendi pencerelerimizi asla hedef almayalim.
    if (targetHwnd == m_overlayWindow.GetHwnd() || targetHwnd == m_hiddenHwnd)
        return false;

    // Orijinal stil/konum bilgisi, kapatirken birebir geri yuklemek icin saklanir.
    const LONG origStyle   = (LONG)(LONG_PTR)GetWindowLongPtrW(targetHwnd, GWL_STYLE);
    const LONG origExStyle = (LONG)(LONG_PTR)GetWindowLongPtrW(targetHwnd, GWL_EXSTYLE);
    RECT origRect = {};
    GetWindowRect(targetHwnd, &origRect);

    HMONITOR   hMon = MonitorFromWindow(targetHwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi  = { sizeof(MONITORINFO) };
    if (!GetMonitorInfoW(hMon, &mi)) return false;

    const int monW = mi.rcMonitor.right  - mi.rcMonitor.left;
    const int monH = mi.rcMonitor.bottom - mi.rcMonitor.top;

    // --- Overlay + renderer hazirla (tum D3D erisimi m_renderMutex altinda) ---
    {
        std::lock_guard<std::mutex> renderLock(m_renderMutex);

        if (!m_overlayWindow.GetHwnd())
        {
            if (!m_overlayWindow.Create(m_hInstance, mi.rcMonitor))
                return false;
            if (!m_renderer.Initialize(m_overlayWindow.GetHwnd(), monW, monH))
            {
                m_overlayWindow.Destroy();
                return false;
            }
        }
        else
        {
            m_overlayWindow.SetMonitorRect(mi.rcMonitor);
            SetWindowPos(m_overlayWindow.GetHwnd(), HWND_TOPMOST,
                mi.rcMonitor.left, mi.rcMonitor.top, monW, monH, SWP_NOACTIVATE);

            if (!m_renderer.IsReady() ||
                !m_renderer.Resize(monW, monH))
            {
                m_renderer.Cleanup();
                if (!m_renderer.Initialize(m_overlayWindow.GetHwnd(), monW, monH))
                    return false;
            }
        }

        m_renderer.InvalidateTextureCache();
    }

    // --- Pencereyi gercek cercevesiz tam ekran yap: WS_POPUP + tam monitor boyutu ---
    LONG newStyle = origStyle;
    newStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU | WS_BORDER | WS_DLGFRAME);
    newStyle |= WS_POPUP;
    SetWindowLongPtrW(targetHwnd, GWL_STYLE, newStyle);

    // Cercere kaldirildigi icin istemci alani tam monitor olmali: kenarliksiz
    // pencere icin istenen DIS boyut = monitor boyutu.
    SetWindowPos(targetHwnd, HWND_NOTOPMOST,
        mi.rcMonitor.left, mi.rcMonitor.top, monW, monH,
        SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_SHOWWINDOW);

    // Stil degisikligini kalici hale getir, sonra basariyla devam et.
    m_origStyle        = origStyle;
    m_origExStyle      = origExStyle;
    m_origWindowRect   = origRect;
    m_windowTransformed.store(true);

    if (!m_captureMgr.StartCapture(targetHwnd, m_renderer.GetDevice()))
    {
        SetWindowLongPtrW(targetHwnd, GWL_STYLE,   m_origStyle);
        SetWindowLongPtrW(targetHwnd, GWL_EXSTYLE, m_origExStyle);
        SetWindowPos(targetHwnd, HWND_NOTOPMOST,
            m_origWindowRect.left, m_origWindowRect.top,
            m_origWindowRect.right  - m_origWindowRect.left,
            m_origWindowRect.bottom - m_origWindowRect.top,
            SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        m_windowTransformed.store(false);
        return false;
    }

    m_overlayWindow.SetTargetWindow(targetHwnd);
    m_overlayWindow.Show(true);

    m_targetHwnd.store(targetHwnd);
    m_lastTextureGeneration = m_captureMgr.GetTextureGeneration();
    m_forceRedraw.store(true);
    m_isActive.store(true);
    return true;
}

void UpscalerApp::StopUpscaling()
{
    if (!m_isActive.exchange(false)) return;

    // 1) Overlay'i gizle ve hedefi birak (artik fare/kare iletilmez).
    m_overlayWindow.SetTargetWindow(nullptr);
    m_overlayWindow.Show(false);

    // 2) Render thread'in mevcut RenderFrame cagrisi bitene kadar bekle.
    {
        std::lock_guard<std::mutex> renderLock(m_renderMutex);
        m_renderer.InvalidateTextureCache();
    }

    // 3) Yakalamayi durdur (render thread artik RenderFrame cagirmiyor).
    m_captureMgr.StopCapture();

    // 4) Hedef pencerenin orijinal stilini/konumunu geri yukle.
    HWND target = m_targetHwnd.load();
    if (m_windowTransformed.load() && target && IsWindow(target))
    {
        SetWindowLongPtrW(target, GWL_STYLE,   m_origStyle);
        SetWindowLongPtrW(target, GWL_EXSTYLE, m_origExStyle);
        SetWindowPos(target, HWND_NOTOPMOST,
            m_origWindowRect.left, m_origWindowRect.top,
            m_origWindowRect.right  - m_origWindowRect.left,
            m_origWindowRect.bottom - m_origWindowRect.top,
            SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    }
    m_windowTransformed.store(false);
    m_targetHwnd.store(nullptr);
}

// ---- Render Loop ------------------------------------------------------------

void UpscalerApp::RenderLoop()
{
    // Yoklama periyodu. Oyun kare urettikce 1 ms (maksimum FPS / en dusuk
    // gecikme), goruntu sabit kaldikca kademeli olarak 8 ms'ye cikar. Boylece
    // bos ekranda BOS SPIN yapilmaz ve islemci/bellek yolu bosuna mesgul edilmez.
    int idlePollMs = 1;

    while (m_isRunning.load())
    {
        if (!m_isActive.load())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            continue;
        }

        HWND target = m_targetHwnd.load();
        if (!target || !IsWindow(target))
        {
            bool stopped = false;
            if (m_toggleMutex.try_lock())
            {
                std::lock_guard<std::mutex> lock(m_toggleMutex, std::adopt_lock);
                if (m_isActive.load())
                {
                    StopUpscaling();
                    stopped = true;
                }
            }
            if (stopped) PlayFeedbackSound(2);
            continue;
        }

        // Hedef pencere odakta degilse (Alt+Tab ile baska uygulamaya gecildi)
        // overlay gizlenir ama yakalama/stil DEGISTIRILMEZ: geri donuste aninda
        // devam edilir. Bu, Alt+Tab kaynakli cokmeleri de onler.
        HWND fg = GetForegroundWindow();
        const bool targetFocused = (fg == target || fg == m_overlayWindow.GetHwnd());
        if (!targetFocused)
        {
            m_overlayWindow.Show(false);
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            continue;
        }

        if (!m_overlayWindow.IsVisible())
            m_overlayWindow.Show(true);

        bool presented = false;

        {
            std::lock_guard<std::mutex> captureLock(m_captureMgr.GetMutex());

            if (m_isActive.load())
            {
                // Zorunlu yeniden cizim (mod/dil degisikligi, cihaz kurtarma).
                const bool forceRedraw = m_forceRedraw.exchange(false);

                // Texture yeniden yaratildiysa renderer'in SRV cache'ini gecersiz kil.
                // (Yalnizca CPU tarafi durum sifirlar; HICBIR GPU isi yapmaz.)
                const uint32_t gen = m_captureMgr.GetTextureGeneration();
                if (gen != m_lastTextureGeneration)
                {
                    std::lock_guard<std::mutex> renderLock(m_renderMutex);
                    m_renderer.InvalidateTextureCache();
                    m_lastTextureGeneration = gen;
                }

                // Yakala + hash'le. Yeni kare YOKSA doku guncellenmez; yani
                // buradan sonra hicbir GPU isi tetiklenmez.
                ID3D11Texture2D* capturedTex = m_captureMgr.GetLatestTexture();

                const int srcW = m_captureMgr.GetTextureWidth();
                const int srcH = m_captureMgr.GetTextureHeight();

                // ------------------------------------------------------------------
                // DIRTY FRAME GARANTISI (Intel UHD %0 mesguliyet):
                // Oyun yeni kare uretmediyse ClearRenderTargetView / Draw /
                // Present cagrilarinin HICBIRI yapilmaz.
                // ------------------------------------------------------------------
                const bool haveFrame = (capturedTex != nullptr) &&
                                       (m_captureMgr.HasNewFrame() || forceRedraw) &&
                                       (srcW > 0 && srcH > 0);

                if (haveFrame)
                {
                    float crop[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
                    m_captureMgr.GetClientCrop(crop);
                    const float uvMin[2] = { crop[0], crop[1] };
                    const float uvMax[2] = { crop[2], crop[3] };

                    const UpscaleMode mode = m_currentMode.load();
                    const float sharpness  = m_sharpness.load();

                    std::lock_guard<std::mutex> renderLock(m_renderMutex);

                    const bool rendered = m_renderer.RenderFrame(
                        capturedTex, srcW, srcH, uvMin, uvMax, mode, sharpness);
                    presented = rendered;

                    if (!rendered && m_renderer.IsDeviceLost())
                    {
                        // GPU cihazi kayboldu (surucu guncellemesi / TDR): sifirdan kur.
                        RECT rc = {};
                        int monW = 0, monH = 0;
                        if (m_overlayWindow.GetHwnd() && GetClientRect(m_overlayWindow.GetHwnd(), &rc))
                        {
                            monW = rc.right - rc.left;
                            monH = rc.bottom - rc.top;
                        }
                        if (monW > 0 && monH > 0)
                        {
                            m_renderer.RecoverDevice(m_overlayWindow.GetHwnd(), monW, monH);
                            m_lastTextureGeneration = 0;
                            m_forceRedraw.store(true);
                        }
                    }
                }
            }
        }

        // Yeni kare sunulduysa hizli kal (yuksek FPS); gosterilecek yeni bir
        // sey yoksa kademeli olarak yavasla (CPU ve iGPU'yu bos birak).
        if (presented)
            idlePollMs = 1;
        else
            idlePollMs = (idlePollMs < 8) ? (idlePollMs + 1) : 8;

        std::this_thread::sleep_for(std::chrono::milliseconds(idlePollMs));
    }
}

// ---- Window Procedure -------------------------------------------------------

LRESULT CALLBACK UpscalerApp::HiddenWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    UpscalerApp* pThis = nullptr;
    if (msg == WM_NCCREATE)
    {
        CREATESTRUCTW* cs = (CREATESTRUCTW*)lParam;
        pThis = (UpscalerApp*)cs->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
    }
    else
    {
        pThis = (UpscalerApp*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    }

    if (pThis)
        return pThis->HandleHiddenMessage(hwnd, msg, wParam, lParam);

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT UpscalerApp::HandleHiddenMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_HOTKEY:
    {
        int hotkeyId = (int)wParam;
        switch (hotkeyId)
        {
        case ID_HOTKEY_TOGGLE:   ToggleUpscaling();            break;
        case ID_HOTKEY_MODE_CAS: SetMode(UpscaleMode::CAS);    break;
        case ID_HOTKEY_MODE_FSR: SetMode(UpscaleMode::FSR);    break;
        case ID_HOTKEY_MODE_INT: SetMode(UpscaleMode::INTEGER);break;
        case ID_HOTKEY_MODE_OFF: SetMode(UpscaleMode::OFF);    break;
        }
        return 0;
    }

    case WM_TRAYICON:
    {
        if (lParam == WM_RBUTTONUP)
        {
            ShowTrayMenu();
        }
        else if (lParam == WM_LBUTTONDBLCLK)
        {
            ShowShortcutsInfo();
        }
        return 0;
    }

    case WM_COMMAND:
    {
        int cmdId = LOWORD(wParam);
        switch (cmdId)
        {
        case ID_MENU_TOGGLE:    ToggleUpscaling();             break;
        case ID_MENU_MODE_CAS:  SetMode(UpscaleMode::CAS);     break;
        case ID_MENU_MODE_FSR:  SetMode(UpscaleMode::FSR);     break;
        case ID_MENU_MODE_INT:  SetMode(UpscaleMode::INTEGER); break;
        case ID_MENU_MODE_OFF:  SetMode(UpscaleMode::OFF);     break;
        case ID_MENU_LANG_TR:   m_english.store(false);        break;
        case ID_MENU_LANG_EN:   m_english.store(true);         break;
        case ID_MENU_ABOUT:     ShowShortcutsInfo();           break;
        case ID_MENU_EXIT:      PostQuitMessage(0);            break;
        }
        return 0;
    }

    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}