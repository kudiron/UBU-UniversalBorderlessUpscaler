#pragma once

#include "Common.h"

class CaptureManager
{
public:
    CaptureManager();
    ~CaptureManager();

    bool StartCapture(HWND targetHwnd, ID3D11Device* d3d11Device);
    void StopCapture();

    ID3D11Texture2D* GetLatestTexture();

    bool HasNewFrame() const { return m_frameChanged; }

    int GetTextureWidth()  const { return m_textureW; }
    int GetTextureHeight() const { return m_textureH; }
    int GetClientWidth()   const { return m_clientW; }
    int GetClientHeight()  const { return m_clientH; }

    void GetClientCrop(float outUV[4]) const;

    HWND GetTargetHwnd() const { return m_targetHwnd; }

    bool IsTargetAlive()     const;
    bool IsTargetMinimized() const;

    std::mutex& GetMutex() { return m_captureMutex; }

    // Texture yeniden yaratildiginda artar. Renderer SRV cache'ini gecersiz kilmak
    // icin kullanilir. Cagiran m_captureMutex tutmalidir.
    uint32_t GetTextureGeneration() const { return m_textureGeneration; }

private:
    bool UpdateClientSize();
    bool CreateCaptureResources();
    void ReleaseCaptureResources();
    bool GrabFrame();

    std::mutex m_captureMutex;

    HWND          m_targetHwnd  = nullptr;
    ID3D11Device* m_d3d11Device = nullptr;
    ID3D11DeviceContext* m_d3d11Context = nullptr;  // cache'lendi

    int m_textureW = 0;
    int m_textureH = 0;
    int m_clientW  = 0;
    int m_clientH  = 0;

    HDC     m_memDC     = nullptr;
    HBITMAP m_bitmap    = nullptr;
    HGDIOBJ m_oldBitmap = nullptr;
    void*   m_bits      = nullptr;
    int     m_bufW      = 0;
    int     m_bufH      = 0;
    int     m_offX      = 0;
    int     m_offY      = 0;

    std::vector<BYTE> m_packBuffer;

    unsigned int m_lastFrameHash = 0;
    bool m_forceUpload  = true;
    bool m_frameChanged = false;

    ID3D11Texture2D* m_captureTexture = nullptr;
    uint32_t m_textureGeneration = 0;  // CreateCaptureResources'ta artar
};