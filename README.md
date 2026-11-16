<div align="center">

# UniversalBorderlessUpscaler (UBU)

### Zayıf PC'ler için hafif, tek dosyalık çerçevesiz tam ekran ve keskinleştirme aracı

*A lightweight, single-file borderless fullscreen and sharpening tool for low-end PCs*

[![Version](https://img.shields.io/badge/version-1.0.0-blue?style=flat-square)](../../releases)
[![Status](https://img.shields.io/badge/status-beta-yellow?style=flat-square)](#bilinen-sorunlar)
[![License: MIT](https://img.shields.io/badge/license-MIT-green?style=flat-square)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows%2010%20%7C%2011-0078D6?style=flat-square&logo=windows)](#sistem-gereksinimleri)
[![DirectX](https://img.shields.io/badge/DirectX-11-107C10?style=flat-square)](#sistem-gereksinimleri)
[![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?style=flat-square&logo=cplusplus)](#kaynak-koddan-derleme)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen?style=flat-square)](#katkıda-bulunma)

[**⬇️ İndir / Download**](../../releases/latest) · [Türkçe](#türkçe) · [English](#english) · [Sorun Bildir / Report a Bug](../../issues)

<!--
  Buraya demo GIF'i veya ekran görüntüsü ekleyin (en etkili iyileştirme):
  Add a demo GIF or screenshot here (the single most effective improvement):

  <img src="docs/demo.gif" alt="UBU demo" width="720">
-->

</div>

> [!WARNING]
> **Beta / Erken aşama:** UBU henüz beta aşamasındadır. FPS kararlılığı ve bazı davranışlar üzerinde çalışılıyor. Ayrıntılar için [Bilinen Sorunlar](#bilinen-sorunlar) bölümüne bakın. Geri bildirim ve katkılar çok değerli: [Katkıda Bulunma](#katkıda-bulunma).
>
> UBU is in beta. FPS stability and some behaviors are still being worked on, see [Known Issues](#known-issues). Feedback and contributions are very welcome: [Contributing](#contributing).

---

## Türkçe

### İçindekiler

- [UBU Nedir?](#ubu-nedir)
- [Neden UBU?](#neden-ubu)
- [Özellikler](#özellikler)
- [Hızlı Başlangıç](#hızlı-başlangıç)
- [Kısayollar ve Keskinleştirme Modları](#kısayollar-ve-keskinleştirme-modları)
- [Sistem Gereksinimleri](#sistem-gereksinimleri)
- [Kurulum](#kurulum)
- [Nasıl Çalışır?](#nasıl-çalışır)
- [Sık Sorulan Sorular](#sık-sorulan-sorular)
- [Sorun Giderme](#sorun-giderme)
- [Bilinen Sorunlar](#bilinen-sorunlar)
- [Yol Haritası](#yol-haritası)
- [Kaynak Koddan Derleme](#kaynak-koddan-derleme)
- [Mimari](#mimari)
- [Katkıda Bulunma](#katkıda-bulunma)
- [Lisans](#lisans)
- [Sorumluluk Reddi](#sorumluluk-reddi)

### UBU Nedir?

**UBU**; oyunları, emülatörleri ve pencereli uygulamaları **gerçek çerçevesiz (borderless) tam ekran** yapar. Görüntüyü GPU üzerinde ölçekleyip keskinleştirir.

Tek bir `UBU.exe` dosyasıdır: kurulum gerektirmez, sistem tepsisinde sessizce çalışır.

### Neden UBU?

| | |
|---|---|
| 🪶 **Hafif** | Tek geçişli, bölmesiz shader ve kirli kare optimizasyonu ile **Intel UHD gibi entegre GPU'ları yormayacak** şekilde tasarlandı. |
| 📦 **Tek dosya** | ~258 KB. Statik CRT, Visual C++ Redistributable gerekmez. Yönetici yetkisi gerekmez. |
| ⚡ **Düşük gecikme** | VSync kilidi olmadan, kare hazır olur olmaz ekrana sunar. |
| 🔓 **Açık kaynak** | MIT lisanslı. İndirmek yerine kendiniz de derleyebilirsiniz. |

<!--
  İsteğe bağlı: ölçümlerinizi buraya ekleyin. Örnek tablo:
  Optional: add your measurements here. Example table:

  | Test sistemi | UBU kapalı | UBU açık |
  |---|---|---|
  | Intel UHD 620 / GPU kullanımı | %XX | %XX |
-->

### Özellikler

| | Özellik | Açıklama |
|---|---|---|
| 🖥️ | **Gerçek Çerçevesiz Tam Ekran** | `Alt+Shift+Z` ile hedef pencerenin başlık çubuğu ve kenarlıkları (`WS_CAPTION`, `WS_THICKFRAME`) kaldırılır; pencere `WS_POPUP` olarak monitörü tamamen kaplar. Kapatınca orijinal stil, boyut ve konum **birebir** geri yüklenir. |
| ⚡ | **VSync Kilidi Olmayan Anlık Sunum** | DirectX 11 swapchain, `DXGI_PRESENT_ALLOW_TEARING` + `Present(0, …)` ile sunulur. Tearing desteklenmeyen sistemlerde otomatik olarak güvenli moda düşer. |
| 🔋 | **Kirli Kare (Dirty Frame) Optimizasyonu** | GPU yalnızca **yeni bir kare üretildiğinde** çalışır. Görüntü sabitse çizim yapılmaz; dizüstü ve Intel UHD sistemlerde güç tüketimi ile ısı azalır. |
| ✨ | **Tek Geçişli Hafif Keskinleştirme** | Varsayılan mod, donanımsal bilinear filtreleme üzerine yalnızca 5 örnekli, bölmesiz, tek geçişli hafif bir keskinleştirme uygular. |
| 🖱️ | **Doğru Fare Eşlemesi** | Fare konumu, tıklama, sürükleme ve tekerlek hareketleri, hedef pencerenin **istemci alanına (client area)** en-boy oranı korunarak aktarılır. |
| 🪟 | **Alt+Tab Güvenliği** | Hedef pencere odaktan çıktığında kaplama gizlenir; pencere stili ve yakalama bozulmaz, geri dönüldüğünde devam eder. |
| 🔁 | **Art Arda Aç/Kapat Dayanıklılığı** | DirectX cihazı ve swapchain kapatmada yok edilmez, yalnızca pencere gizlenir. Onlarca kez `Alt+Shift+Z` basmak güvenlidir. |
| 🌐 | **Çift Dilli Arayüz** | Sistem tepsisi menüsü **Türkçe / English** olarak değiştirilebilir. |

### Hızlı Başlangıç

1. [Releases](../../releases/latest) sayfasından `UBU.exe` dosyasını indirin.
2. Çift tıklayın. Program sistem tepsisinde (sağ alt) sessizce başlar.
3. Oyunu, emülatörü veya uygulamayı **pencereli modda** açıp ön plana getirin.
4. `Alt + Shift + Z` tuşlarına basın. Pencere kenarlıksız tam ekran olur.
5. Tekrar `Alt + Shift + Z` ile pencere orijinal haline döner.
6. Çıkmak için tepsi simgesine sağ tıklayıp **Çıkış (Güvenli Kapatma)** seçin.

> [!TIP]
> Sistem tepsisindeki UBU simgesine **sağ tıklayarak** menüye, **çift tıklayarak** kısayol bilgilerine ulaşabilirsiniz.

### Kısayollar ve Keskinleştirme Modları

| Kısayol | Mod | Ne işe yarar? |
|---|---|---|
| `Alt` + `Shift` + `Z` | Tam ekran | Çerçevesiz tam ekranı **aç / kapat** |
| `Alt` + `Shift` + `1` | **CAS** *(varsayılan)* | Hafif ve dengeli keskinleştirme. Çoğu durum için önerilir. |
| `Alt` + `Shift` + `2` | **FSR 1.0** | Maksimum kenar netliği (EASU + RCAS). Daha fazla GPU gücü kullanır. |
| `Alt` + `Shift` + `3` | **Integer** | Retro / 2D oyunlar için keskin pikseller. |
| `Alt` + `Shift` + `0` | **Kapalı** | Yalnızca donanım bilinear filtreleme (en hızlı). |

### Sistem Gereksinimleri

| Bileşen | Gereksinim |
|---|---|
| **İşletim Sistemi** | Windows 10 64-bit (1809 veya üzeri önerilir) / Windows 11 |
| **Grafik** | DirectX 11 uyumlu GPU (Intel UHD Graphics dahil), `ps_5_0` desteği yeterlidir |
| **İşlemci** | Herhangi bir x86-64 işlemci |
| **Ek** | DirectX 11 çalışma zamanı (Windows ile gelir), `d3dcompiler_47.dll` |

### Kurulum

Kurulum gerekmez. [Releases](../../releases/latest) bölümünden `UBU.exe` dosyasını indirip istediğiniz klasörden çalıştırmanız yeterlidir.

> [!NOT]
> UBU tek kopya (single-instance) olarak çalışır. İkinci kez başlatılırsa uyarı verir ve kapanır.

**Doğrulama:** İndirdiğiniz dosyanın SHA256 değerini Releases sayfasındaki asset satırında görebilirsiniz. PowerShell ile kontrol etmek için:

```powershell
Get-FileHash .\UBU.exe -Algorithm SHA256
```

### Nasıl Çalışır?

```text
Hedef pencere ──► Yakalama ──► Kirli kare kontrolü ──► GPU ölçekleme + keskinleştirme ──► Tam ekran kaplama
 (pencereli)     (PrintWindow/   (kare değişti mi?)      (D3D11, tek geçişli shader)       (fare girişi geri
                   BitBlt)                                                                  hedefe aktarılır)
```

1. Hedef pencerenin görüntüsü yakalanır ve D3D11 dokusuna aktarılır.
2. Kare bir öncekiyle aynıysa çizim atlanır (GPU boşta kalır).
3. Yeni kare, seçilen modla (CAS / FSR 1.0 / Integer / Kapalı) ölçeklenip en-boy oranı korunarak monitöre sunulur.
4. Tam ekrandaki fare hareketleri, hedef pencerenin istemci alanına dönüştürülerek oyuna iletilir.

### Sık Sorulan Sorular

<details>
<summary><b>Antivirüs programım UBU.exe'yi tehdit olarak işaretledi. Güvenli mi?</b></summary>

UBU, başka pencerelerin stilini değiştirir ve görüntülerini yakalar. İmzasız küçük `.exe` dosyaları ve bu tür davranışlar bazı antivirüslerde yanlış alarma (false positive) yol açabilir. Kaynak kodu bu depoda açıktır; güvenmiyorsanız [kendiniz derleyebilirsiniz](#kaynak-koddan-derleme). Ayrıca [SHA256 değerini](#kurulum) doğrulayabilirsiniz.

</details>

<details>
<summary><b>Çok oyunculu oyunlarda kullanabilir miyim?</b></summary>

Önerilmez. EAC, BattlEye, Vanguard gibi çekirdek düzeyi hile korumaları dış pencere müdahalesini şüpheli bulabilir. Kullanmadan önce oyunun kullanım koşullarını kontrol edin. Bkz. [Sorumluluk Reddi](#sorumluluk-reddi).

</details>

<details>
<summary><b>Oyunum tam ekran modunda, UBU çalışır mı?</b></summary>

Hayır, oyunu **pencereli moda** alın. UBU pencereli uygulamalar için tasarlanmıştır.

</details>

<details>
<summary><b>Yönetici olarak çalıştırmam gerekir mi?</b></summary>

Genelde hayır. Yalnızca yönetici olarak çalışan bir oyunu yakalamak istiyorsanız UBU'yu da yönetici olarak başlatmanız gerekebilir.

</details>

### Sorun Giderme

| Belirti | Çözüm |
|---|---|
| `Alt+Shift+Z` tepki vermiyor | Aynı kısayolu kullanan başka bir uygulamayı kapatın. UBU açılışta çakışma uyarısı gösterir. |
| Görüntü gelmiyor / siyah ekran | Programı yönetici olarak çalıştırmayı deneyin (bazı oyunlar `PrintWindow`'u kısıtlar). Tam ekran yerine pencereli mod kullanın. |
| Fare konumu kayıyor | Oyun içi çözünürlük ile pencere iç alanı farklı olabilir; oyunu "pencereli" moda alın. |
| FPS artmıyor | Oyunda VSync'i kapatın. UBU tarafında VSync zaten devre dışıdır. |

### Bilinen Sorunlar

UBU şu an beta aşamasındadır. Bilinen sınırlamalar:

- **FPS kararsızlığı:** Bazı sistemlerde ve oyunlarda kare hızı dalgalanabilir veya beklenenden düşük olabilir.
- **Yakalama yöntemi:** Görüntü `PrintWindow` / `BitBlt` ile yakalanıyor. Bazı oyunlarda (özellikle DirectX/Vulkan ile çizenlerde) siyah ekran veya düşük performans görülebilir.
- **Fare eşlemesi:** Oyun içi çözünürlük ile pencere iç alanı farklıysa konum kayabilir.
- **Hile koruması:** Çekirdek düzeyi koruması olan oyunlarda kullanım önerilmez.

Başka bir sorun bulduysanız [Issues](../../issues) bölümünden bildirebilirsiniz.

### Yol Haritası

Bunlar kesin söz değil, araştırılacak veya katkıya açık fikirlerdir:

- [ ] FPS kararsızlığının kaynağını bulmak (profil çıkarma)
- [ ] Daha hızlı yakalama yöntemleri denemek (Windows Graphics Capture, DXGI Desktop Duplication)
- [ ] Farklı oyun, emülatör ve GPU'larda uyumluluk testleri
- [ ] Performans ölçümleri (GPU kullanımı, güç tüketimi) ve README'de yayınlanması
- [ ] GitHub Actions ile otomatik derleme
- [ ] `winget` / Scoop paketi (kararlı sürümden sonra)

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

### Katkıda Bulunma

Katkılar çok değerli. Özellikle şu alanlarda yardım arıyoruz:

- **Performans:** FPS kararsızlığının kaynağını bulmak, profil çıkarmak, daha hızlı bir yakalama yöntemi denemek.
- **Uyumluluk testleri:** Farklı oyun, emülatör ve GPU'larda deneyip sonuçları paylaşmak.
- **Hata düzeltmeleri** ve genel kod iyileştirmeleri.

**Nasıl katkı verirsiniz?**

1. Depoyu fork'layın ve ayrı bir dal (branch) açın.
2. Değişikliğinizi yapıp [Kaynak Koddan Derleme](#kaynak-koddan-derleme) adımlarıyla test edin.
3. Pull Request açın ve neyi, neden değiştirdiğinizi kısaca yazın.

**Hata bildirirken** lütfen şunları ekleyin: Windows sürümü, GPU modeli, oyun/uygulama adı ve ne olduğunu açıklayan kısa bir tarif.

Test sonucu paylaşmak da bir katkıdır: hangi oyun/GPU'da çalıştığını veya çalışmadığını [Issues](../../issues) üzerinden yazabilirsiniz.

### Lisans

Bu proje **MIT Lisansı** ile lisanslanmıştır. Ayrıntı için [LICENSE](LICENSE) dosyasına bakın.

### Sorumluluk Reddi

> [!WARNING]
> UBU, pencere stillerini geçici olarak değiştirir. Bazı çekirdek düzeyi hile korumaları (EAC, BattlEye, Vanguard vb.) dış pencere müdahalesini şüpheli karşılayabilir. Çok oyunculu oyunlarda kullanmadan önce oyunun kullanım koşullarını kontrol edin. Kullanım sorumluluğu kullanıcıya aittir.

<div align="right"><a href="#universalborderlessupscaler-ubu">↑ Başa dön</a></div>

---

## English

### Table of Contents

- [What is UBU?](#what-is-ubu)
- [Why UBU?](#why-ubu)
- [Features](#features)
- [Quick Start](#quick-start)
- [Shortcuts and Sharpening Modes](#shortcuts-and-sharpening-modes)
- [Requirements](#requirements)
- [Installation](#installation)
- [FAQ](#faq)
- [Known Issues](#known-issues)
- [Roadmap](#roadmap)
- [Build from Source](#build-from-source)
- [Contributing](#contributing)
- [License](#license)

### What is UBU?

**UBU** turns games, emulators and windowed applications into **true borderless fullscreen**, upscaling and sharpening the image on the GPU.

It ships as a single `UBU.exe`: no installation, it lives quietly in the system tray.

### Why UBU?

| | |
|---|---|
| 🪶 **Lightweight** | Single-pass, division-free shader and dirty-frame skipping, designed to stay light on **integrated GPUs such as Intel UHD**. |
| 📦 **Single file** | ~258 KB. Statically linked CRT, no Visual C++ Redistributable, no admin rights. |
| ⚡ **Low latency** | No VSync lock: frames are presented as soon as they are ready. |
| 🔓 **Open source** | MIT licensed. Prefer not to trust a binary? Build it yourself. |

### Features

- 🖥️ **True Borderless Fullscreen**: `Alt+Shift+Z` strips `WS_CAPTION` / `WS_THICKFRAME` and makes the target window a `WS_POPUP` covering the whole monitor. When toggled off, the original style, size and position are restored **exactly**.
- ⚡ **Zero-VSync Presentation**: the DirectX 11 swapchain uses `DXGI_PRESENT_ALLOW_TEARING` with `Present(0, …)`. Falls back safely on hardware without tearing support.
- 🔋 **Dirty Frame Optimization**: the GPU only draws when a genuinely new frame is available.
- ✨ **Single-Pass Lightweight Sharpening**: a 5-tap, division-free default shader on top of hardware bilinear filtering.
- 🖱️ **Accurate Mouse Mapping**: cursor position, clicks, drags and wheel events are forwarded to the target **client area** with aspect-ratio correction.
- 🪟 **Alt+Tab Safe**: the overlay hides when the target loses focus; window style and capture state stay intact.
- 🔁 **Toggle-Spam Resilient**: the D3D device and swapchain are kept alive on toggle-off, so repeated `Alt+Shift+Z` presses are safe.
- 🌐 **Bilingual UI**: tray menu available in **Turkish / English**.

### Quick Start

1. Download `UBU.exe` from the [latest release](../../releases/latest).
2. Run it. It starts silently in the system tray.
3. Open your game, emulator or app in **windowed mode** and bring it to the foreground.
4. Press `Alt + Shift + Z` to go borderless fullscreen.
5. Press it again to restore the original window.
6. Right-click the tray icon and choose **Exit** to quit.

### Shortcuts and Sharpening Modes

| Shortcut | Mode | Description |
|---|---|---|
| `Alt` + `Shift` + `Z` | Fullscreen | Toggle borderless fullscreen |
| `Alt` + `Shift` + `1` | **CAS** *(default)* | Light and balanced sharpening. Recommended for most cases. |
| `Alt` + `Shift` + `2` | **FSR 1.0** | Maximum edge sharpness (EASU + RCAS). Uses more GPU. |
| `Alt` + `Shift` + `3` | **Integer** | Crisp pixels for retro / 2D games. |
| `Alt` + `Shift` + `0` | **Off** | Pure hardware bilinear (fastest). |

### Requirements

- Windows 10 64-bit (1809+) or Windows 11
- DirectX 11 capable GPU (Intel UHD Graphics included), `ps_5_0` support
- Any x86-64 CPU

### Installation

No installation needed. Download `UBU.exe` from [Releases](../../releases/latest) and run it from any folder. You can verify the download with the SHA256 shown on the release page:

```powershell
Get-FileHash .\UBU.exe -Algorithm SHA256
```

### FAQ

<details>
<summary><b>My antivirus flags UBU.exe. Is it safe?</b></summary>

UBU modifies other windows' styles and captures their content. Small unsigned executables doing this can trigger false positives. The full source is in this repo, so you can [build it yourself](#build-from-source) and verify the SHA256.

</details>

<details>
<summary><b>Can I use it in multiplayer games?</b></summary>

Not recommended. Kernel-level anti-cheat (EAC, BattlEye, Vanguard) may treat external window manipulation as suspicious. Check the game's terms first.

</details>

<details>
<summary><b>My game is in exclusive fullscreen. Will it work?</b></summary>

No. Switch the game to **windowed mode**; UBU is built for windowed applications.

</details>

### Known Issues

UBU is currently in beta. Known limitations:

- **FPS instability:** On some systems and games the frame rate may fluctuate or be lower than expected.
- **Capture method:** Frames are captured with `PrintWindow` / `BitBlt`. Some games (especially DirectX/Vulkan renderers) may show a black screen or reduced performance.
- **Mouse mapping:** The cursor may drift if the in-game resolution differs from the window's client area.
- **Anti-cheat:** Not recommended for games with kernel-level protection.

Found something else? Please open an [issue](../../issues).

### Roadmap

Ideas to investigate and open for contributions, not promises:

- [ ] Find the cause of FPS instability (profiling)
- [ ] Try faster capture paths (Windows Graphics Capture, DXGI Desktop Duplication)
- [ ] Compatibility testing across games, emulators and GPUs
- [ ] Publish performance measurements (GPU usage, power draw)
- [ ] Automated builds with GitHub Actions
- [ ] `winget` / Scoop package (after a stable release)

### Build from Source

```bat
git clone https://github.com/kudiron/UBU-UniversalBorderlessUpscaler.git
cd UBU-UniversalBorderlessUpscaler

cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

`UBU.exe` is copied to the project root automatically. Requires CMake ≥ 3.20 and Visual Studio / MSVC Build Tools with the Windows 10/11 SDK.

### Contributing

Contributions are very welcome. Areas where help is especially appreciated:

- **Performance:** finding the cause of FPS instability, profiling, trying a faster capture path.
- **Compatibility testing:** trying UBU with different games, emulators and GPUs and sharing results.
- **Bug fixes** and general code improvements.

**How to contribute**

1. Fork the repo and create a feature branch.
2. Make your change and test it using the [Build from Source](#build-from-source) steps.
3. Open a Pull Request describing what you changed and why.

**When reporting a bug**, please include your Windows version, GPU model, the game/app name, and a short description of what happened. Sharing a test result (works / doesn't work on game X with GPU Y) is a contribution too.

### License

Released under the **MIT License**. See [LICENSE](LICENSE).

<div align="right"><a href="#universalborderlessupscaler-ubu">↑ Back to top</a></div>
