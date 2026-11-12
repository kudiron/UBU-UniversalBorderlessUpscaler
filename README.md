<div align="center">

# UniversalBorderlessUpscaler (UBU)

**Pencereli oyunları ve uygulamaları gerçek çerçevesiz tam ekrana çeviren, GPU üzerinde ölçekleyip keskinleştiren hafif Windows aracı.**

*Turns windowed games and apps into true borderless fullscreen, with lightweight GPU upscaling and sharpening.*

[![Version](https://img.shields.io/badge/version-1.0.0-blue?style=flat-square)](../../releases)
[![License: MIT](https://img.shields.io/badge/license-MIT-green?style=flat-square)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows%2010%20%7C%2011-0078D6?style=flat-square&logo=windows)](#sistem-gereksinimleri)
[![DirectX](https://img.shields.io/badge/DirectX-11-107C10?style=flat-square)](#sistem-gereksinimleri)
[![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?style=flat-square&logo=cplusplus)](#kaynak-koddan-derleme)
[![Portable](https://img.shields.io/badge/portable-single%20exe-orange?style=flat-square)](#kurulum)

[**İndir / Download**](../../releases) · [Türkçe](#türkçe) · [English](#english)

</div>

---

## Türkçe

### İçindekiler

- [Hakkında](#hakkında)
- [Özellikler](#özellikler)
- [Kısayollar](#kısayollar)
- [Hızlı Başlangıç](#hızlı-başlangıç)
- [Sistem Gereksinimleri](#sistem-gereksinimleri)
- [Kurulum](#kurulum)
- [Kaynak Koddan Derleme](#kaynak-koddan-derleme)
- [Mimari](#mimari)
- [Sorun Giderme](#sorun-giderme)
- [Lisans](#lisans)
- [Sorumluluk Reddi](#sorumluluk-reddi)

### Hakkında

**UBU**; oyunları, emülatörleri ve pencereli uygulamaları **gerçek çerçevesiz (borderless) tam ekran** yapar. Görüntüyü GPU üzerinde ölçekleyip keskinleştirirken **Intel UHD gibi düşük güçlü entegre grafik birimlerini yormayacak** şekilde tasarlanmıştır.

Tek bir `UBU.exe` dosyasıdır. Kurulum gerektirmez ve sistem tepsisinde sessizce çalışır.

### Özellikler

| | Özellik | Açıklama |
|---|---|---|
| 🖥️ | **Gerçek Çerçevesiz Tam Ekran** | `Alt+Shift+Z` ile hedef pencerenin başlık çubuğu ve kenarlıkları (`WS_CAPTION`, `WS_THICKFRAME`) kaldırılır; pencere `WS_POPUP` olarak monitörü tamamen kaplar. Kapatınca orijinal stil, boyut ve konum **birebir** geri yüklenir. |
| ⚡ | **VSync Kilidi Olmayan Anlık Sunum** | DirectX 11 swapchain, `DXGI_PRESENT_ALLOW_TEARING` + `Present(0, …)` ile sunulur. Kare hazır olur olmaz ekrana gider; compositor beklemesi ve VSync kaynaklı gecikme ortadan kalkar. Tearing desteklenmeyen sistemlerde otomatik olarak güvenli moda düşer. |
| 🔋 | **Dirty Frame (Kirli Kare) Optimizasyonu** | GPU yalnızca **yeni bir kare üretildiğinde** çalışır. Görüntü sabitse çizim yapılmaz; dizüstü ve Intel UHD sistemlerde güç tüketimi ile ısı belirgin şekilde azalır. |
| ✨ | **Tek Geçişli Hafif Keskinleştirme** | Varsayılan mod, donanımsal bilinear filtreleme üzerine yalnızca 5 örnekli, bölmesiz, tek geçişli hafif bir keskinleştirme uygular. |
| 🖱️ | **Kusursuz Fare Eşlemesi** | Fare konumu, tıklama, sürükleme ve tekerlek hareketleri, hedef pencerenin **istemci alanına (client area)** en-boy oranı korunarak birebir aktarılır. Başlık çubuğu veya kenarlık kaynaklı kayma olmaz. |
| 🪟 | **Alt+Tab Güvenliği** | Hedef pencere odaktan çıktığında kaplama gizlenir; pencere stili ve yakalama bozulmaz, geri dönüldüğünde anında devam eder. |
| 🔁 | **Art Arda Aç/Kapat Dayanıklılığı** | DirectX cihazı ve swapchain kapatmada yok edilmez, yalnızca pencere gizlenir. Onlarca kez `Alt+Shift+Z` basmak güvenlidir. |
| 🌐 | **Çift Dilli Arayüz** | Sistem tepsisi menüsü **Türkçe / English** olarak değiştirilebilir. |
| 📦 | **Taşınabilir Tek Dosya** | Statik CRT ile derlenir (`/MT`); Visual C++ Redistributable gerekmez. Yönetici yetkisi gerekmez. |

### Kısayollar

| Kısayol | Görev |
|---|---|
| `Alt` + `Shift` + `Z` | Çerçevesiz tam ekranı **aç / kapat** |
| `Alt` + `Shift` + `1` | **CAS**: hafif ve dengeli *(varsayılan, önerilen)* |
| `Alt` + `Shift` + `2` | **FSR 1.0**: maksimum kenar netliği (EASU + RCAS) |
| `Alt` + `Shift` + `3` | **Integer**: retro / 2D için keskin pikseller |
| `Alt` + `Shift` + `0` | Keskinleştirme **kapalı**: yalnızca donanım bilinear (en hızlı) |

> 💡 Sistem tepsisindeki UBU simgesine **sağ tıklayarak** menüye, **çift tıklayarak** kısayol bilgilerine ulaşabilirsiniz.

### Hızlı Başlangıç

1. `UBU.exe` dosyasına çift tıklayın. Program sistem tepsisinde (sağ alt) sessizce başlar.
2. Oyunu, emülatörü veya uygulamayı **pencereli modda** açıp ön plana getirin.
3. `Alt + Shift + Z` tuşlarına basın. Pencere anında kenarlıksız tam ekran olur.
4. Tekrar `Alt + Shift + Z` ile pencere orijinal boyutuna döner.
5. Çıkmak için tepsi simgesine sağ tıklayıp **Çıkış (Güvenli Kapatma)** seçin.

### Sistem Gereksinimleri

| Bileşen | Gereksinim |
|---|---|
| **İşletim Sistemi** | Windows 10 64-bit (1809 veya üzeri önerilir) / Windows 11 |
| **Grafik** | DirectX 11 uyumlu GPU (Intel UHD Graphics dahil), `ps_5_0` desteği yeterlidir |
| **İşlemci** | Herhangi bir x86-64 işlemci |
| **Ek** | DirectX 11 çalışma zamanı (Windows ile gelir), `d3dcompiler_47.dll` |

### Kurulum

Kurulum gerekmez. [Releases](../../releases) bölümünden `UBU.exe` dosyasını indirip istediğiniz klasörden çalıştırmanız yeterlidir.

> [!NOTE]
> UBU tek kopya (single-instance) olarak çalışır. İkinci kez başlatılırsa uyarı verir ve kapanır.

### Kaynak Koddan Derleme

**Gereksinimler:** CMake ≥ 3.20, Visual Studio 2019/2022/2026 (*C++ ile Masaüstü Geliştirme* iş yükü) veya MSVC Build Tools, Windows 10/11 SDK.

```bat
git clone https://github.com/kudiron/UBU-UniversalBorderlessUpscaler.git
cd UBU-UniversalBorderlessUpscaler

cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

Derleme sonunda `UBU.exe` proje kök dizinine otomatik kopyalanır.

<details>
<summary>Ninja ile derleme</summary>

```bat
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

</details>

### Mimari

```text
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

**İş parçacığı modeli:** Ana iş parçacığı pencere mesajlarını, kısayolları ve tepsi menüsünü işler. İkinci bir render iş parçacığı yakalama ve çizim yapar. Paylaşılan durum, atomik değişkenler ile `m_toggleMutex`, `m_renderMutex` ve `m_captureMutex` kilitleriyle korunur.

### Sorun Giderme

| Belirti | Çözüm |
|---|---|
| `Alt+Shift+Z` tepki vermiyor | Aynı kısayolu kullanan başka bir uygulamayı kapatın. UBU açılışta çakışma uyarısı gösterir. |
| Görüntü gelmiyor / siyah ekran | Programı yönetici olarak çalıştırmayı deneyin (bazı oyunlar `PrintWindow`'u kısıtlar). Tam ekran yerine pencereli mod kullanın. |
| Fare konumu kayıyor | Oyun içi çözünürlük ile pencere iç alanı farklı olabilir; oyunu "pencereli" moda alın. |
| FPS artmıyor | Oyunda VSync'i kapatın. UBU tarafında VSync zaten devre dışıdır. |

### Lisans

Bu proje **MIT Lisansı** ile lisanslanmıştır. Ayrıntı için [LICENSE](LICENSE) dosyasına bakın.

### Sorumluluk Reddi

> [!WARNING]
> UBU, pencere stillerini geçici olarak değiştirir. Bazı çekirdek düzeyi hile korumaları (EAC, BattlEye, Vanguard vb.) dış pencere müdahalesini şüpheli karşılayabilir. Çok oyunculu oyunlarda kullanmadan önce oyunun kullanım koşullarını kontrol edin. Kullanım sorumluluğu kullanıcıya aittir.

<div align="right"><a href="#universalborderlessupscaler-ubu">↑ Başa dön</a></div>

---

## English

### Table of Contents

- [About](#about)
- [Features](#features)
- [Shortcuts](#shortcuts)
- [Quick Start](#quick-start)
- [Requirements](#requirements)
- [Installation](#installation)
- [Build from Source](#build-from-source)
- [License](#license)

### About

**UBU** turns games, emulators and windowed applications into **true borderless fullscreen**, upscaling and sharpening the image on the GPU while **staying light enough for integrated graphics such as Intel UHD**.

It ships as a single `UBU.exe`: no installation, no background services. It lives in the system tray.

### Features

- 🖥️ **True Borderless Fullscreen**: `Alt+Shift+Z` strips `WS_CAPTION` / `WS_THICKFRAME` and makes the target window a `WS_POPUP` covering the whole monitor. When toggled off, the original style, size and position are restored **exactly**.
- ⚡ **Zero-VSync, Low-Latency Presentation**: the DirectX 11 swapchain uses `DXGI_PRESENT_ALLOW_TEARING` with `Present(0, …)`, so frames reach the display as soon as they are ready. Falls back safely on hardware without tearing support.
- 🔋 **Dirty Frame Optimization**: the GPU only draws when a genuinely new frame is available; idle screens cost nothing.
- ✨ **Single-Pass Lightweight Sharpening**: a 5-tap, division-free default shader on top of hardware bilinear filtering.
- 🖱️ **Pixel-Perfect Mouse Mapping**: cursor position, clicks, drags and wheel events are forwarded to the target **client area** with aspect-ratio correction.
- 🪟 **Alt+Tab Safe**: the overlay hides when the target loses focus; window style and capture state stay intact and resume instantly.
- 🔁 **Toggle-Spam Resilient**: the D3D device and swapchain are kept alive on toggle-off, so repeated `Alt+Shift+Z` presses are safe.
- 🌐 **Bilingual UI**: tray menu available in **Turkish / English**.
- 📦 **Portable Single File**: statically linked CRT (`/MT`), no Visual C++ Redistributable, no administrator rights required.

### Shortcuts

| Shortcut | Action |
|---|---|
| `Alt` + `Shift` + `Z` | Toggle borderless fullscreen |
| `Alt` + `Shift` + `1` | **CAS**: light & balanced *(default)* |
| `Alt` + `Shift` + `2` | **FSR 1.0**: maximum edge sharpness |
| `Alt` + `Shift` + `3` | **Integer**: crisp pixels for retro / 2D |
| `Alt` + `Shift` + `0` | Sharpening **off**: pure hardware bilinear |

### Quick Start

1. Run `UBU.exe`. It starts silently in the system tray.
2. Open your game, emulator or app in **windowed mode** and bring it to the foreground.
3. Press `Alt + Shift + Z` to go borderless fullscreen.
4. Press it again to restore the original window.
5. Right-click the tray icon and choose **Exit** to quit.

### Requirements

- Windows 10 64-bit (1809+) or Windows 11
- DirectX 11 capable GPU (Intel UHD Graphics included), `ps_5_0` support
- Any x86-64 CPU

### Installation

No installation needed. Download `UBU.exe` from [Releases](../../releases) and run it from any folder.

### Build from Source

```bat
git clone https://github.com/kudiron/UBU-UniversalBorderlessUpscaler.git
cd UBU-UniversalBorderlessUpscaler

cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

`UBU.exe` is copied to the project root automatically.

### License

Released under the **MIT License**. See [LICENSE](LICENSE).

<div align="right"><a href="#universalborderlessupscaler-ubu">↑ Back to top</a></div>
