#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <dwmapi.h>
#include <shellapi.h>

#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <cmath>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")

enum class UpscaleMode : int
{
    OFF = 0,        // Mode 0: Bilinear (No sharpening)
    CAS = 1,        // Mode 1: AMD FidelityFX CAS (Contrast Adaptive Sharpening)
    FSR = 2,        // Mode 2: AMD FidelityFX FSR 1.0 (EASU + RCAS)
    INTEGER = 3     // Mode 3: Pixel-Art Crisp Integer Scaling
};

inline const wchar_t* GetUpscaleModeName(UpscaleMode mode)
{
    switch (mode)
    {
    case UpscaleMode::OFF:
        return L"Kapali (Bilinear)";
    case UpscaleMode::CAS:
        return L"AMD FidelityFX CAS (Hafif ve Dengeli)";
    case UpscaleMode::FSR:
        return L"AMD FSR 1.0 (Maksimum Kenar Netligi)";
    case UpscaleMode::INTEGER:
        return L"Integer / Keskin Piksel (Retro & 2D)";
    default:
        return L"Bilinmeyen Mod";
    }
}

struct alignas(16) ShaderConstants
{
    float srcWidth;
    float srcHeight;
    float invSrcWidth;
    float invSrcHeight;

    float dstWidth;
    float dstHeight;
    float invDstWidth;
    float invDstHeight;

    float uvMin[2];      // Crop bounds: (uMin, vMin) - removes title bar & borders
    float uvMax[2];      // Crop bounds: (uMax, vMax)

    float sharpness;     // 0.0 - 1.0
    int   mode;          // 1 = CAS, 2 = FSR, 3 = INTEGER
    float padding[2];
};

template <typename T>
inline void SafeRelease(T*& ptr)
{
    if (ptr)
    {
        ptr->Release();
        ptr = nullptr;
    }
}
