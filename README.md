# UniversalBorderlessUpscaler (UBU) v1.0

**Dil / Language:** [Türkçe](#türkçe) · [English](#english)

---

## Türkçe

**UniversalBorderlessUpscaler (UBU)**; oyunları, emülatörleri ve pencereli uygulamaları
**gerçek çerçevesiz (borderless) tam ekran** yapan, görüntüyü GPU üzerinde ölçekleyip
keskinleştiren, **Intel UHD gibi düşük güçlü entegre grafik birimlerini yormayan**
hafif bir Windows yardımcı programıdır.

Tek bir `UBU.exe` dosyasıdır; kurulum gerektirmez, arka planda sistem tepsisinde çalışır.

### Öne Çıkan Özellikler

| Özellik | Açıklama |
|---|---|
| **Gerçek Çerçevesiz Tam Ekran** | `Alt+Shift+Z` ile hedef pencerenin başlık çubuğu ve kenarlıkları (`WS_CAPTION`, `WS_THICKFRAME`) kaldırılır, pencere `WS_POPUP` olarak monitörü tamamen kaplar. Kapatınca orijinal stil, boyut ve konum **birebir** geri yüklenir. |
| **VSync Kilidi Olmayan Anlık Sunum** | DirectX 11 swapchain `DXGI_PRESENT_ALLOW_TEARING` + `Present(0, …)` ile sunulur. Böylece kare, hazır olur olmaz ekrana gider; compositor beklemesi ve VSync kaynaklı gecikme ortadan kalkar. Tearing desteklenmeyen sistemlerde otomatik olarak güvenli moda düşer. |
| **Dirty Frame (Kirli Kare) Optimizasyonu** | GPU yalnızca **yeni bir kare üretildiğinde** çalışır. Görüntü sabitse çizim yapılmaz; dizüstü/Intel UHD sistemlerde güç tüketimi ve ısı belirgin şekilde azalır. |
| **Tek Geçişli Hafif Keskinleştirme** | Varsayılan mod, donanımsal bilinear filtreleme üzerine yalnızca 5 örnekli, bölmesiz tek geçişli hafif keskinleştirme uygular. |
| **Kusursuz Fare Eşlemesi** | Tam ekrandaki fare konumu, tıklama, sürükleme ve tekerlek hareketleri hedef pencerenin **istemci alanına (client area)** en-boy oranı korunarak birebir aktarılır. Başlık çubuğu/kenarlık kaynaklı kayma olmaz. |
| **Alt+Tab Güvenliği** | Hedef pencere odaktan çıktığında kaplama gizlenir; pencere stili ve yakalama bozulmaz, geri dönüldüğünde anında devam eder. |
| **Art Arda Aç/Kapat Dayanıklılığı** | DirectX cihazı ve swapchain kapatmada yok edilmez; sadece pencere gizlenir. Onlarca kez `Alt+Shift+Z` basmak güvenlidir. |
| **Çift Dilli Arayüz** | Sistem tepsisi menüsü **Türkçe / English** olarak değiştirilebilir. |
| **Taşınabilir Tek Dosya** | Statik CRT ile derlenir (`/MT`); Visual C++ Redistributable gerekmez. Yönetici yetkisi gerekmez. |

### Kısayollar

| Kısayol | Görev |
|---|---|
| `Alt + Shift + Z` | Çerçevesiz Tam Ekran **Aç / Kapat** |
| `Alt + Shift + 1` | Keskinleştirme modu: **CAS** — Hafif ve Dengeli *(varsayılan, önerilen)* |
| `Alt + Shift + 2` | Keskinleştirme modu: **FSR 1.0** — Maksimum kenar netliği (EASU + RCAS) |
| `Alt + Shift + 3` | Keskinleştirme modu: **Integer** — Retro / 2D için keskin pikseller |
| `Alt + Shift + 0` | Keskinleştirme **Kapalı** — yalnızca donanım bilinear (en hızlı) |

> Sistem tepsisindeki UBU simgesine **sağ tıklayarak** menüye, **çift tıklayarak** kısayol bilgilerine ulaşabilirsiniz.

### Nasıl Kullanılır?

1. `UBU.exe` dosyasına çift tıklayın. Program sağ alt sistem tepsisinde sessizce başlar.
2. Oyunu / emülatörü / uygulamayı pencereli modda açıp **ön plana** getirin.
3. `Alt + Shift + Z` tuşlarına basın: pencere anında kenarlıksız tam ekran olur.
4. Tekrar `Alt + Shift + Z`: pencere orijinal boyutuna döner.
5. Çıkmak için tepsi simgesine sağ tıklayıp **Çıkış (Güvenli Kapatma)** seçin.

### Sistem Gereksinimleri

- **İşletim Sistemi:** Windows 10 64-bit (1809 veya üzeri önerilir) / Windows 11
- **Grafik:** DirectX 11 uyumlu GPU (Intel UHD Graphics dahil) — `ps_5_0` desteği yeterlidir
- **İşlemci:** Herhangi bir x86-64 işlemci
- **Ek:** DirectX 11 çalışma zamanı (Windows ile birlikte gelir), `d3dcompiler_47.dll`

### Kurulum

Kurulum gerekmez. [Releases](../../releases) bölümünden `UBU.exe` dosyasını indirip
istediğiniz klasöre koyup çalıştırmanız yeterlidir.

> **Not:** Tek kopya çalışır (single-instance). İkinci kez başlatılırsa uyarı verir ve kapanır.

### Kaynak Koddan Derleme

**Gereksinimler:** CMake ≥ 3.20, Visual Studio 2019/2022/2026 (C++ Masaüstü Geliştirme iş yükü) veya MSVC Build Tools, Windows 10/11 SDK.

```bat
git clone https://github.com
cd UBU-UniversalBorderlessUpscaler

cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

Derleme sonunda `UBU.exe` proje kök dizinine otomatik kopyalanır.
(Ninja ile derlemek isterseniz: `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build`)

### Mimari Özet

```
src/
├── Main.cpp            # Giriş noktası, tek kopya koruması, DPI farkındalığı, özel istisna filtresi
├── UpscalerApp.*       # Kısayollar, sistem tepsisi, başlat/durdur, render döngüsü
├── OverlayWindow.*     # Tam ekran kaplaması + fare girişi yönlendirme (en-boy oranlı)
├── CaptureManager.*    # PrintWindow/BitBlt → D3D11 dokusu, kirli kare (hash) tespiti
├── D3D11Renderer.*     # Swapchain, tearing, shader yönetimi, letterbox ölçekleme
├── ShaderSources.h     # Gömülü (fallback) HLSL kaynağı
└── Shaders/
    └── MainShaders.hlsl
```

**İş parçacığı modeli:** Ana iş parçacığı pencere mesajlarını, kısayolları ve tepsi
menüsünü işler; ikinci bir render iş parçacığı yakalama + çizim yapar. Paylaşılan
durum atomik değişkenler ve `m_toggleMutex` / `m_renderMutex` / `m_captureMutex`
kilitleri ile korunur.

### Sorun Giderme

| Belirti | Çözüm |
|---|---|
| `Alt+Shift+Z` tepki vermiyor | Aynı kısayolu kullanan başka bir uygulamayı kapatın. UBU açılışta çakışma uyarısı gösterir. |
| Görüntü gelmiyor / siyah | Programı yönetici olarak çalıştırmayı deneyin (bazı oyunlar `PrintWindow`'u kısıtlar). Tam ekran yerine pencereli mod kullanın. |
| Fare konumu kayıyor | Oyun içi çözünürlük ile pencere iç alanı farklı olabilir; oyunu "pencereli" moda alın. |
| FPS artmıyor | Oyunda VSync'i kapatın. UBU tarafında VSync zaten devre dışıdır. |

### Lisans

Bu proje **MIT Lisansı** ile lisanslanmıştır. Ayrıntı için [LICENSE](LICENSE) dosyasına bakın.

### Sorumluluk Reddi

UBU, pencere stillerini geçici olarak değiştirir. Bazı çekirdek düzeyi hile korumaları
(EAC, BattlEye, Vanguard vb.) dış pencere müdahalesini şüpheli karşılayabilir.
Çok oyunculu oyunlarda kullanmadan önce oyunun kullanım koşullarını kontrol edin.
Kullanım sorumluluğu kullanıcıya aittir.

---

## English

**UniversalBorderlessUpscaler (UBU)** turns games, emulators and windowed applications into
**true borderless fullscreen**, upscaling and sharpening the image on the GPU while
**staying light enough for integrated graphics such as Intel UHD**.

It ships as a single `UBU.exe` — no installation, no background services; it lives in the
system tray.

### Features

- **True Borderless Fullscreen** — `Alt+Shift+Z` strips `WS_CAPTION` / `WS_THICKFRAME` and
  makes the target window a `WS_POPUP` covering the whole monitor. On toggle-off the original
  style, size and position are restored **exactly**.
- **Zero-VSync, Tearing-Free Presentation** — the DirectX 11 swapchain uses
  `DXGI_PRESENT_ALLOW_TEARING` with `Present(0, …)`, so frames reach the display as soon as
  they are ready. Falls back safely on hardware without tearing support.
- **Dirty Frame Optimisation** — the GPU only draws when a genuinely new frame is available;
  idle screens cost nothing.
- **Single-Pass Lightweight Sharpening** — 5-tap, division-free default shader on top of
  hardware bilinear filtering.
- **Pixel-Perfect Mouse Mapping** — cursor position, clicks, drags and wheel events are
  forwarded to the target **client area** with aspect-ratio correction.
- **Alt+Tab Safe** — the overlay hides when the target loses focus; window style and capture
  state stay intact and resume instantly.
- **Bilingual UI** — tray menu available in **Turkish / English**.
- **Portable Single File** — statically linked CRT (`/MT`), no Visual C++ Redistributable,
  no administrator rights required.

### Shortcuts

| Shortcut | Action |
|---|---|
| `Alt + Shift + Z` | Toggle borderless fullscreen |
| `Alt + Shift + 1` | Sharpening: **CAS** — light & balanced *(default)* |
| `Alt + Shift + 2` | Sharpening: **FSR 1.0** — maximum edge sharpness |
| `Alt + Shift + 3` | Sharpening: **Integer** — crisp pixels for retro / 2D |
| `Alt + Shift + 0` | Sharpening **Off** — pure hardware bilinear |

### Requirements

- Windows 10 64-bit (1809+) or Windows 11
- DirectX 11 capable GPU (Intel UHD Graphics included), `ps_5_0` support
- Any x86-64 CPU

### Build from Source

```bat
git clone https://github.com
cd UBU-UniversalBorderlessUpscaler

cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

`UBU.exe` is copied to the project root automatically.

### License

Released under the **MIT License**. See [LICENSE](LICENSE).
