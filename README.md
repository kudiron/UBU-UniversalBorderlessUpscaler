<div align="center">

# UniversalBorderlessUpscaler (UBU)

**Zayıf PC'ler için hafif, tek dosyalık çerçevesiz tam ekran ve keskinleştirme aracı**

**Current documented release: `v1.0.2`**
<br>
*A lightweight, single-file borderless fullscreen and sharpening tool for low-end PCs*

[![Release](https://img.shields.io/github/v/release/kudiron/UBU-UniversalBorderlessUpscaler?style=flat-square&label=version)](../../releases/latest)
[![License: MIT](https://img.shields.io/badge/license-MIT-green?style=flat-square)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows%2010%20%7C%2011-0078D6?style=flat-square&logo=windows)](#sistem-gereksinimleri)
[![DirectX](https://img.shields.io/badge/DirectX-11-107C10?style=flat-square)](#sistem-gereksinimleri)
[![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?style=flat-square&logo=cplusplus)](#kaynak-koddan-derleme)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen?style=flat-square)](#katkıda-bulunma)

[**⬇ İndir / Download**](../../releases/latest) · [Türkçe](#türkçe) · [English](#english) · [Sorun Bildir / Report a Bug](../../issues)

<img src="before-after.png?v=2" alt="UBU kapalı ve açık karşılaştırması" width="900">

</div>

---

## Türkçe

### İçindekiler

- [Genel Bakış](#genel-bakış)
- [Özellikler](#özellikler)
- [Hızlı Başlangıç](#hızlı-başlangıç)
- [Kısayollar ve Keskinleştirme Modları](#kısayollar-ve-keskinleştirme-modları)
- [Sistem Gereksinimleri](#sistem-gereksinimleri)
- [Kurulum](#kurulum)
- [Nasıl Çalışır?](#nasıl-çalışır)
- [Sık Sorulan Sorular](#sık-sorulan-sorular)
- [Sorun Giderme](#sorun-giderme)
- [Bilinen Sınırlamalar](#bilinen-sınırlamalar)
- [Yol Haritası](#yol-haritası)
- [Kaynak Koddan Derleme](#kaynak-koddan-derleme)
- [Mimari](#mimari)
- [Katkıda Bulunma](#katkıda-bulunma)
- [Lisans](#lisans)
- [Sorumluluk Reddi](#sorumluluk-reddi)

### Genel Bakış

UBU; oyunları, emülatörleri ve pencereli uygulamaları **gerçek çerçevesiz (borderless) tam ekran** yapar. Görüntüyü GPU üzerinde ölçekleyip keskinleştirir.

Tek bir `UBU.exe` dosyasıdır. Kurulum gerektirmez, sistem tepsisinde sessizce çalışır.

| | |
|---|---|
| **Hafif** | Tek geçişli, bölmesiz shader ve kirli kare optimizasyonu ile Intel UHD gibi entegre GPU'ları yormayacak şekilde tasarlandı. |
| **Tek dosya** | ~258 KB. Statik CRT, Visual C++ Redistributable gerekmez. Yönetici yetkisi gerekmez. |
| **Düşük gecikme** | VSync kilidi olmadan, kare hazır olur olmaz ekrana sunar. |
| **Açık kaynak** | MIT lisanslı. İndirmek yerine kendiniz de derleyebilirsiniz. |

### Özellikler

- **Gerçek çerçevesiz tam ekran (FPS dostu).** `Alt+Shift+Z` ile aktif pencere yakalanır; her kare GPU'da monitör boyutuna ölçeklenir. Hedef pencerenin boyutu ve stili değiştirilmez: oyun düşük çözünürlükte (örn. 960×540) çalışmaya devam eder ve yüksek FPS'ini korur.
- **Tearing destekli sunum.** DirectX 11 swapchain, `DXGI_PRESENT_ALLOW_TEARING` + `Present(0, …)` ile sunulur. Tearing desteklenmeyen sistemlerde otomatik olarak güvenli moda düşer.
- **Kirli kare (dirty frame) optimizasyonu.** GPU yalnızca yeni bir kare üretildiğinde çalışır. Görüntü sabitse çizim yapılmaz; dizüstü ve Intel UHD sistemlerde güç tüketimi ile ısı azalır.
- **Tek geçişli hafif keskinleştirme.** Varsayılan mod, donanımsal bilinear filtreleme üzerine 5 örnekli, bölmesiz, tek geçişli bir keskinleştirme uygular.
- **Piksel-hassas fare eşlemesi.** Fare konumu, tıklama, sürükleme ve tekerlek hareketleri; tam ekrandaki konumdan hedef pencerenin **istemci alanına (client area)** en-boy oranı korunarak (letterbox/pillarbox telafili) ölçeklenip aktarılır.
- **Odak ve küçültme güvenliği.** Hedef pencere odaktan çıktığında veya simge durumuna küçültüldüğünde kaplama gizlenir; pencere geri geldiğinde yakalama motoru yeniden başlatılmadan devam eder.
- **Art arda aç/kapat dayanıklılığı.** DirectX cihazı ve swapchain kapatmada yok edilmez. Onlarca kez `Alt+Shift+Z` basmak güvenlidir.
- **Çift dilli arayüz.** Sistem tepsisi menüsü Türkçe / English olarak değiştirilebilir.

### v1.0.2 — Hata Düzeltmeleri ve Kararlılık

UBU v1.0.2, ölçeklenmiş tam ekran modunda fare etkileşimini düzeltmeye ve pencere davranışı ile kare zamanlamasını daha kararlı hâle getirmeye odaklanan bir hata düzeltme güncellemesidir.

- **Fare koordinatlarının doğru eşleştirilmesi:** `Alt+Shift+Z` ile ölçeklenmiş tam ekran etkinleştirildiğinde imleç konumu hedef oyun penceresinin koordinatlarıyla doğru şekilde eşleştirilir.
- **Buton tıklamalarının doğru iletilmesi:** Oyun içi menü ve arayüz tıklamaları doğrudan hedef pencereye yönlendirilir; imlecin beklenmedik konumlara sıçraması önlenir.
- **Küçültülen pencerede otomatik uyku:** Hedef pencere `IsIconic` ile simge durumunda algılandığında overlay gizlenir ve gereksiz CPU/GPU işlemleri durdurulur.
- **Hızlı geri yükleme:** Hedef pencere yeniden öne getirildiğinde yakalama motoru baştan oluşturulmadan overlay önceki konumuna ve ölçeğine geri döner.
- **Odak kararlılığı:** Ölçeklenmiş tam ekran overlay'ine tıklandığında oyunun odağını kaybederek arka plana düşmesi engellenir.
- **Daha kararlı kare zamanlaması:** Yakalama ve DirectX 11 çizim döngüleri arasındaki senkronizasyon iyileştirilerek mikro takılmaların (micro-stutter) azaltılması hedeflenmiştir.

### Hızlı Başlangıç

1. [Releases](../../releases/latest) sayfasından `UBU.exe` dosyasını indirin.
2. Çift tıklayın. Program sistem tepsisinde (sağ alt) sessizce başlar.
3. Oyunu, emülatörü veya uygulamayı **pencereli modda** açıp ön plana getirin.
4. `Alt` + `Shift` + `Z` tuşlarına basın. Pencere kenarlıksız tam ekran olur.
5. Tekrar basarak normale dönün.
6. Çıkmak için tepsi simgesine sağ tıklayıp **Çıkış (Güvenli Kapatma)** seçin.

> [!TIP]
> Tepsi simgesine **sağ tıklayarak** menüye, **çift tıklayarak** kısayol bilgilerine ulaşabilirsiniz.

### Kısayollar ve Keskinleştirme Modları

| Kısayol | Mod | Açıklama |
|---|---|---|
| `Alt` + `Shift` + `Z` | Tam ekran | Çerçevesiz tam ekranı aç / kapat |
| `Alt` + `Shift` + `1` | **CAS** *(varsayılan)* | Hafif ve dengeli keskinleştirme. Çoğu durum için önerilir. |
| `Alt` + `Shift` + `2` | **FSR 1.0** | Maksimum kenar netliği (EASU + RCAS). Daha fazla GPU gücü kullanır. |
| `Alt` + `Shift` + `3` | **Integer** | Retro / 2D oyunlar için keskin pikseller. |
| `Alt` + `Shift` + `0` | **Kapalı** | Yalnızca donanım bilinear filtreleme (en hızlı). |

### Sistem Gereksinimleri

| Bileşen | Gereksinim |
|---|---|
| **İşletim sistemi** | Windows 10 64-bit (1809 veya üzeri önerilir) / Windows 11 |
| **Grafik** | DirectX 11 uyumlu GPU (Intel UHD Graphics dahil), `ps_5_0` desteği yeterlidir |
| **İşlemci** | Herhangi bir x86-64 işlemci |
| **Ek** | DirectX 11 çalışma zamanı (Windows ile gelir), `d3dcompiler_47.dll` |

### Kurulum

Kurulum gerekmez. [Releases](../../releases/latest) bölümünden `UBU.exe` dosyasını indirip istediğiniz klasörden çalıştırmanız yeterlidir.

> [!NOTE]
> UBU tek kopya (single-instance) olarak çalışır. İkinci kez başlatılırsa uyarı verir ve kapanır.

**Doğrulama.** İndirdiğiniz dosyanın SHA256 değerini Releases sayfasındaki asset satırında görebilirsiniz. PowerShell ile kontrol etmek için:

```powershell
Get-FileHash .\UBU.exe -Algorithm SHA256
```

### Nasıl Çalışır?

```text
Hedef pencere ──► Yakalama ──► Kirli kare kontrolü ──► GPU ölçekleme + keskinleştirme ──► Tam ekran kaplama
 (pencereli)     (PrintWindow/   (kare değişti mi?)      (D3D11, tek geçişli shader)       (fare girişi geri
                   BitBlt)                                                                  hedefe aktarılır)
```

1. Hedef pencere **olduğu gibi bırakılır** (düşük çözünürlükte kalır, FPS kazancı korunur). Görüntüsü yakalanıp D3D11 dokusuna aktarılır.
2. Kare bir öncekiyle aynıysa çizim atlanır, GPU boşta kalır.
3. Yeni kare, seçilen modla (CAS / FSR 1.0 / Integer / Kapalı) ölçeklenip en-boy oranı korunarak monitöre sunulur.
4. Tam ekrandaki fare hareketleri, hedef pencerenin istemci alanına ölçeklenerek oyuna iletilir.

### Sık Sorulan Sorular

<details>
<summary><b>Antivirüs programım UBU.exe'yi tehdit olarak işaretledi. Güvenli mi?</b></summary>

UBU, başka pencerelerin görüntüsünü yakalar ve onlara girdi iletir. İmzasız küçük `.exe` dosyaları ve bu tür davranışlar bazı antivirüslerde yanlış alarma (false positive) yol açabilir. Kaynak kodun tamamı bu depoda açıktır; isterseniz [kendiniz derleyebilir](#kaynak-koddan-derleme) ve [SHA256 değerini](#kurulum) doğrulayabilirsiniz.

</details>

<details>
<summary><b>Çok oyunculu oyunlarda kullanabilir miyim?</b></summary>

Önerilmez. EAC, BattlEye, Vanguard gibi çekirdek düzeyi hile korumaları dış pencere müdahalesini şüpheli bulabilir. Kullanmadan önce oyunun kullanım koşullarını kontrol edin. Bkz. [Sorumluluk Reddi](#sorumluluk-reddi).

</details>

<details>
<summary><b>Oyunum özel tam ekran (exclusive fullscreen) modunda, UBU çalışır mı?</b></summary>

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
| Fare konumu kayıyor | Oyunu pencereli moda alın. Fareyi **Raw Input** ile okuyan bazı oyunlar harici imleç yönlendirmesini yok sayabilir. |
| FPS düşüyor veya dalgalanıyor | `Alt+Shift+1` (CAS) ya da `Alt+Shift+0` (Kapalı) moduna geçin; FSR 1.0 daha fazla GPU gücü kullanır. Oyunda VSync'i kapatın. |

### Bilinen Sınırlamalar

UBU şu anda **v1.0.2 (kararlı)** sürümündedir.

- **Yakalama yöntemi.** Görüntü `PrintWindow` / `BitBlt` ile yakalanır. Bazı oyunlarda (özellikle DirectX/Vulkan ile çizenlerde) siyah ekran görülebilir.
- **Hile koruması.** Çekirdek düzeyi koruması olan oyunlarda kullanım önerilmez.
- **FPS.** Yakalama ve ölçekleme işlemi küçük FPS düşüşlerine veya dalgalanmalarına yol açabilir; etkisi oyuna, GPU'ya ve seçilen moda (en hafifi Kapalı/CAS, en ağırı FSR 1.0) göre değişir. Bu konu üzerinde çalışılıyor, bkz. [Yol Haritası](#yol-haritası).

Başka bir sorun bulduysanız [Issues](../../issues) bölümünden bildirebilirsiniz.

### Yol Haritası

Bunlar kesin söz değil, araştırılacak veya katkıya açık fikirlerdir.

- [ ] Kalan FPS kararsızlığının kaynağını bulmak (profil çıkarma)
- [ ] Daha hızlı yakalama yöntemleri denemek (Windows Graphics Capture, DXGI Desktop Duplication)
- [ ] Farklı oyun, emülatör ve GPU'larda uyumluluk testleri
- [ ] Performans ölçümleri (GPU kullanımı, güç tüketimi) ve yayınlanması
- [ ] GitHub Actions ile otomatik derleme
- [ ] `winget` / Scoop paketi

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

**İş parçacığı modeli.** Ana iş parçacığı pencere mesajlarını, kısayolları ve tepsi menüsünü işler. İkinci bir render iş parçacığı yakalama ve çizim yapar. Paylaşılan durum, atomik değişkenler ile `m_toggleMutex`, `m_renderMutex` ve `m_captureMutex` kilitleriyle korunur.

### Katkıda Bulunma

Katkılar çok değerlidir. Özellikle şu alanlarda yardım arıyoruz:

- **Performans:** FPS kararsızlığının kaynağını bulmak, profil çıkarmak, daha hızlı bir yakalama yöntemi denemek.
- **Uyumluluk testleri:** Farklı oyun, emülatör ve GPU'larda deneyip sonuçları paylaşmak.
- **Hata düzeltmeleri** ve genel kod iyileştirmeleri.

**Nasıl katkı verirsiniz?**

1. Depoyu fork'layın ve ayrı bir dal (branch) açın.
2. Değişikliğinizi yapıp [derleme adımlarıyla](#kaynak-koddan-derleme) test edin.
3. Pull Request açın ve neyi, neden değiştirdiğinizi kısaca yazın.

**Hata bildirirken** lütfen şunları ekleyin: Windows sürümü, GPU modeli, oyun/uygulama adı ve ne olduğunu açıklayan kısa bir tarif. Test sonucu paylaşmak da bir katkıdır: hangi oyun/GPU'da çalıştığını veya çalışmadığını [Issues](../../issues) üzerinden yazabilirsiniz.

### Lisans

Bu proje **MIT Lisansı** ile lisanslanmıştır. Ayrıntı için [LICENSE](LICENSE) dosyasına bakın.

### Sorumluluk Reddi

> [!WARNING]
> UBU, başka uygulamaların pencerelerini yakalar ve onlarla etkileşime girer. Bazı çekirdek düzeyi hile korumaları (EAC, BattlEye, Vanguard vb.) bunu şüpheli karşılayabilir. Çok oyunculu oyunlarda kullanmadan önce oyunun kullanım koşullarını kontrol edin. Kullanım sorumluluğu kullanıcıya aittir.

<div align="right"><a href="#universalborderlessupscaler-ubu">↑ Başa dön</a></div>

---

## English

### Table of Contents

- [Overview](#overview)
- [Features](#features)
- [v1.0.2 — Bug Fixes and Stability Improvements](#v102--bug-fixes-and-stability-improvements)
- [Quick Start](#quick-start)
- [Shortcuts and Sharpening Modes](#shortcuts-and-sharpening-modes)
- [Requirements](#requirements)
- [Installation](#installation)
- [How It Works](#how-it-works)
- [FAQ](#faq)
- [Troubleshooting](#troubleshooting)
- [Known Limitations](#known-limitations)
- [Roadmap](#roadmap)
- [Building from Source](#building-from-source)
- [Architecture](#architecture)
- [Contributing](#contributing)
- [License](#license)
- [Disclaimer](#disclaimer)

### Overview

UBU turns games, emulators and windowed applications into **true borderless fullscreen**, scaling and sharpening the image on the GPU.

It ships as a single `UBU.exe`. No installation is required, and it runs quietly in the system tray.

| | |
|---|---|
| **Lightweight** | Single-pass, division-free shader and dirty-frame skipping, designed to stay light on integrated GPUs such as Intel UHD. |
| **Single file** | ~258 KB. Statically linked CRT, no Visual C++ Redistributable, no administrator rights. |
| **Low latency** | No VSync lock: frames are presented as soon as they are ready. |
| **Open source** | MIT licensed. Prefer not to trust a binary? Build it yourself. |

### Features

- **True borderless fullscreen (FPS-friendly).** `Alt+Shift+Z` captures the active window and scales each frame to the monitor on the GPU. The target window's size and style are not modified, so the game keeps rendering at its low resolution (for example 960×540) and retains its high FPS.
- **Tearing-capable presentation.** The DirectX 11 swapchain uses `DXGI_PRESENT_ALLOW_TEARING` with `Present(0, …)`, and falls back to a safe mode on systems without tearing support.
- **Dirty-frame optimization.** The GPU draws only when a new frame is produced. A static image costs nothing, which lowers power draw and heat on laptops and Intel UHD systems.
- **Lightweight single-pass sharpening.** The default mode applies a 5-tap, division-free sharpen on top of hardware bilinear filtering.
- **Pixel-accurate mouse mapping.** Cursor position, clicks, drags and wheel events are scaled from the fullscreen overlay to the target window's **client area**, preserving the aspect ratio (letterbox/pillarbox compensated).
- **Focus and minimize handling.** The overlay hides when the target loses focus or is minimized, then resumes without restarting the capture engine.
- **Toggle-spam resilient.** The D3D device and swapchain are kept alive when toggled off, so pressing `Alt+Shift+Z` repeatedly is safe.
- **Bilingual UI.** The tray menu can be switched between Turkish and English.

### v1.0.2 — Bug Fixes and Stability Improvements

UBU v1.0.2 is a bug-fix release focused on correcting mouse interaction in scaled fullscreen mode and improving window behavior and frame pacing.

- **Accurate mouse coordinate mapping:** When scaled fullscreen is enabled with `Alt+Shift+Z`, cursor coordinates are correctly mapped to the target game window.
- **Reliable button clicks:** Click messages from in-game menus and interfaces are forwarded directly to the target window, preventing unexpected cursor jumps.
- **Automatic idle behavior for minimized windows:** When the target window is detected as minimized through `IsIconic`, the overlay is hidden and unnecessary CPU/GPU work is suspended.
- **Fast restore:** When the target window returns to the foreground, the overlay restores its previous position and scale without rebuilding the capture engine from scratch.
- **Focus stability:** Clicking the scaled fullscreen overlay no longer causes the game to lose focus and fall into the background.
- **Improved frame pacing:** Synchronization between the capture and DirectX 11 rendering loops has been improved to reduce micro-stutter.

### Quick Start

1. Download `UBU.exe` from the [latest release](../../releases/latest).
2. Run it. It starts silently in the system tray (bottom right).
3. Open your game, emulator or app in **windowed mode** and bring it to the foreground.
4. Press `Alt` + `Shift` + `Z` to go borderless fullscreen.
5. Press it again to return to the normal window.
6. To quit, right-click the tray icon and choose **Exit**.

> [!TIP]
> Right-click the tray icon for the menu; double-click it to see the shortcut reference.

### Shortcuts and Sharpening Modes

| Shortcut | Mode | Description |
|---|---|---|
| `Alt` + `Shift` + `Z` | Fullscreen | Toggle borderless fullscreen |
| `Alt` + `Shift` + `1` | **CAS** *(default)* | Light, balanced sharpening. Recommended for most cases. |
| `Alt` + `Shift` + `2` | **FSR 1.0** | Maximum edge clarity (EASU + RCAS). Uses more GPU. |
| `Alt` + `Shift` + `3` | **Integer** | Crisp pixels for retro and 2D games. |
| `Alt` + `Shift` + `0` | **Off** | Hardware bilinear filtering only (fastest). |

### Requirements

| Component | Requirement |
|---|---|
| **OS** | Windows 10 64-bit (1809 or later recommended) or Windows 11 |
| **GPU** | DirectX 11 capable GPU (Intel UHD Graphics included) with `ps_5_0` support |
| **CPU** | Any x86-64 processor |
| **Runtime** | DirectX 11 runtime (ships with Windows), `d3dcompiler_47.dll` |

### Installation

No installation is needed. Download `UBU.exe` from [Releases](../../releases/latest) and run it from any folder.

> [!NOTE]
> UBU runs as a single instance. Launching a second copy shows a warning and exits.

**Verify the download.** The SHA256 hash is listed next to the asset on the release page. To check it in PowerShell:

```powershell
Get-FileHash .\UBU.exe -Algorithm SHA256
```

### How It Works

```text
Target window ──► Capture ──► Dirty-frame check ──► GPU scale + sharpen ──► Fullscreen overlay
 (windowed)     (PrintWindow/   (did the frame       (D3D11, single-pass      (mouse input is
                  BitBlt)         change?)             shader)                  forwarded back)
```

1. The target window is left **as is** (it stays at its low resolution, preserving the FPS gain). Its image is captured and uploaded to a D3D11 texture.
2. If the frame is identical to the previous one, drawing is skipped and the GPU stays idle.
3. A new frame is scaled with the selected mode (CAS / FSR 1.0 / Integer / Off) and presented to the monitor with the aspect ratio preserved.
4. Mouse input on the fullscreen overlay is scaled to the target's client area and forwarded to the game.

### FAQ

<details>
<summary><b>My antivirus flags UBU.exe. Is it safe?</b></summary>

UBU captures the contents of other windows and forwards input to them. Small unsigned executables that do this can trigger false positives. The full source is in this repository, so you can [build it yourself](#building-from-source) and verify the [SHA256 hash](#installation).

</details>

<details>
<summary><b>Can I use it in multiplayer games?</b></summary>

Not recommended. Kernel-level anti-cheat systems (EAC, BattlEye, Vanguard) may treat external window interaction as suspicious. Check the game's terms of service first. See the [Disclaimer](#disclaimer).

</details>

<details>
<summary><b>My game is in exclusive fullscreen. Will it work?</b></summary>

No. Switch the game to **windowed mode**; UBU is built for windowed applications.

</details>

<details>
<summary><b>Do I need to run it as administrator?</b></summary>

Usually not. You may need to if the game you want to capture is itself running as administrator.

</details>

### Troubleshooting

| Symptom | Solution |
|---|---|
| `Alt+Shift+Z` does nothing | Close any other application that uses the same shortcut. UBU shows a conflict warning on startup. |
| Black screen / no image | Try running UBU as administrator (some games restrict `PrintWindow`). Use windowed mode instead of fullscreen. |
| Mouse position drifts | Switch the game to windowed mode. Games that read the mouse via **Raw Input** may ignore external cursor redirection. |
| FPS drops or fluctuates | Switch to `Alt+Shift+1` (CAS) or `Alt+Shift+0` (Off); FSR 1.0 uses more GPU. Disable VSync in the game. |

### Known Limitations

UBU is currently at **v1.0.2 (stable)**.

- **Capture method.** Frames are captured with `PrintWindow` / `BitBlt`. Some games, especially DirectX/Vulkan renderers, may show a black screen.
- **Anti-cheat.** Not recommended for games with kernel-level protection.
- **FPS.** Capturing and scaling the window can cause small FPS drops or fluctuations. The impact depends on the game, the GPU and the selected mode (lightest: Off/CAS, heaviest: FSR 1.0). This is being worked on, see the [Roadmap](#roadmap-1).

Found something else? Please open an [issue](../../issues).

### Roadmap

These are ideas to investigate and areas open for contribution, not promises.

- [ ] Profile and find the source of remaining FPS instability
- [ ] Try faster capture paths (Windows Graphics Capture, DXGI Desktop Duplication)
- [ ] Compatibility testing across games, emulators and GPUs
- [ ] Publish performance measurements (GPU usage, power draw)
- [ ] Automated builds with GitHub Actions
- [ ] `winget` / Scoop package

### Building from Source

**Requirements:** CMake ≥ 3.20, Visual Studio 2019/2022/2026 (*Desktop development with C++* workload) or MSVC Build Tools, and the Windows 10/11 SDK.

```bat
git clone https://github.com/kudiron/UBU-UniversalBorderlessUpscaler.git
cd UBU-UniversalBorderlessUpscaler

cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

`UBU.exe` is copied to the project root automatically.

<details>
<summary>Building with Ninja</summary>

```bat
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

</details>

### Architecture

```text
src/
├── Main.cpp            # Entry point, single-instance guard, DPI awareness, custom exception filter
├── UpscalerApp.*       # Hotkeys, tray icon, start/stop, render loop
├── OverlayWindow.*     # Fullscreen overlay + aspect-correct mouse input forwarding
├── CaptureManager.*    # PrintWindow/BitBlt → D3D11 texture, dirty-frame (hash) detection
├── D3D11Renderer.*     # Swapchain, tearing, shader management, letterbox scaling
├── ShaderSources.h     # Embedded (fallback) HLSL source
└── Shaders/
    └── MainShaders.hlsl
```

**Threading model.** The main thread handles window messages, hotkeys and the tray menu. A second render thread performs capture and drawing. Shared state is protected by atomics and by the `m_toggleMutex`, `m_renderMutex` and `m_captureMutex` locks.

### Contributing

Contributions are very welcome. Help is especially appreciated in these areas:

- **Performance:** finding the cause of FPS instability, profiling, trying a faster capture path.
- **Compatibility testing:** trying UBU with different games, emulators and GPUs, and sharing the results.
- **Bug fixes** and general code improvements.

**How to contribute**

1. Fork the repository and create a feature branch.
2. Make your change and test it using the [build steps](#building-from-source).
3. Open a Pull Request describing what you changed and why.

**When reporting a bug,** please include your Windows version, GPU model, the game or app name, and a short description of what happened. Sharing a test result (works / doesn't work on game X with GPU Y) is a contribution too.

### License

Released under the **MIT License**. See [LICENSE](LICENSE) for details.

### Disclaimer

> [!WARNING]
> UBU captures and interacts with other applications' windows. Some kernel-level anti-cheat systems (EAC, BattlEye, Vanguard, etc.) may treat this as suspicious. Check the game's terms of service before using UBU in multiplayer games. Use at your own risk.

<div align="right"><a href="#universalborderlessupscaler-ubu">↑ Back to top</a></div>
