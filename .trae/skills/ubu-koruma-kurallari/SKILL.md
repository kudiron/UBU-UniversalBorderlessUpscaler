---
name: ubu-koruma-kurallari
description: UBU projesinde çalışan DPI, fare ve Intel UHD FPS ayarlarını korur.
---

# UBU Koruma Kuralları
- wWinMain girişindeki PerMonitorV2 DPI farkındalığını ve fiziksel monitör koordinat hesaplamasını asla bozma.
- Intel UHD ekran kartı için WS_EX_NOREDIRECTIONBITMAP, VSync kapalı (ALLOW_TEARING) ve Dirty Frame çizim optimizasyonunu koru.
- Her kod değişikliğinden sonra projeyi Release modunda derle ve proje kök dizinine güncel UBU.exe üret.