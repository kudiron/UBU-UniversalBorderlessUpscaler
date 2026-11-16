#include "Common.h"
#include "UpscalerApp.h"

// ---- Global exception filter -------------------------------------------------
// KRITIK: EXCEPTION_CONTINUE_EXECUTION dondurmek, hatali komutu tekrar tekrar
// calistirip sonsuz MessageBox dongusune yol acar. EXCEPTION_CONTINUE_SEARCH ile
// Windows process'i temiz kapatir. Kullaniciya bilgi verir, sonra cikilir.
static LONG WINAPI UBUExceptionFilter(EXCEPTION_POINTERS* /*ep*/)
{
    MessageBoxW(
        nullptr,
        L"UBU beklenmedik bir hatayla karşılaştı ve kapatılacak.\n"
        L"Lütfen UBU'yu yeniden başlatın.",
        L"UBU - Hata",
        MB_OK | MB_ICONERROR
    );
    return EXCEPTION_CONTINUE_SEARCH;
}

// ---- Entry points ------------------------------------------------------------

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, PWSTR /*pCmdLine*/, int /*nCmdShow*/)
{
    // ---------------------------------------------------------------------
    // KRITIK / ADIM 1: DPI farkindaligi EN ILK islem olmali.
    // Hicbir pencere, hicbir koordinat sorgusu yapilmadan once cagrilirsa
    // Windows bize TUM koordinatlari (GetClientRect, ClientToScreen, fare
    // mesajlari, monitor rect) %150 olceklendirme altinda bile HAM FIZIKSEL
    // piksel olarak verir. Boylece "sagdaki butona basinca sola gitme"
    // (DPI olcekleme kaymasi) hatasi tamamen ortadan kalkar.
    // ---------------------------------------------------------------------
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    SetUnhandledExceptionFilter(UBUExceptionFilter);

    // 1 ms yuksek cozunurluklu Windows zamanlayicisi (sleep_for / Sleep gecikmelerini onler, FPS'i ucurur)
    timeBeginPeriod(1);

    // Single-instance guard
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"UniversalBorderlessUpscaler_SingleInstance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        MessageBoxW(
            nullptr,
            L"Universal Borderless Upscaler (UBU) zaten arka planda calisiyor!\n"
            L"Sistem tepsisindeki simgeyi kullanabilir veya Alt+Shift+Z tuslarina basabilirsiniz.",
            L"UBU Bilgi",
            MB_OK | MB_ICONINFORMATION
        );
        if (hMutex) CloseHandle(hMutex);
        timeEndPeriod(1);
        return 0;
    }

    // Process priority: NORMAL. UBU'yu ABOVE_NORMAL yapmak, yakalama thread'inin
    // oyunun render thread'ini CPU'dan ac birakmasi ve dogrudan FPS dususu
    // yaratmasi demekti. UBU normal oncelikte oyunla esit yarisir; render
    // thread'i ayrica BELOW_NORMAL'e cekilir (bkz. UpscalerApp::Initialize).
    SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

    {
        UpscalerApp app;
        if (app.Initialize(hInstance))
        {
            app.Run();
            app.Shutdown();
        }
    }

    timeEndPeriod(1);

    if (hMutex)
    {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }

    return 0;
}

// console entry-point (Visual Studio linkage fallback)
int main(int /*argc*/, char* /*argv*/[])
{
    return wWinMain(GetModuleHandle(nullptr), nullptr, GetCommandLineW(), SW_SHOW);
}