#include "CaptureManager.h"

#include <cstring>

#ifndef PW_RENDERFULLCONTENT
#define PW_RENDERFULLCONTENT 0x00000002
#endif

CaptureManager::CaptureManager() {}
CaptureManager::~CaptureManager() { StopCapture(); }

// ----- Resources -------------------------------------------------------------

bool CaptureManager::CreateCaptureResources()
{
    if (m_textureW <= 0 || m_textureH <= 0) return false;
    if (m_bufW <= 0 || m_bufH <= 0) return false;
    if (!m_d3d11Device) return false;

    // Yarim kalan onceki denemeden kalan kaynak olabilir; temiz basla.
    ReleaseCaptureResources();

    D3D11_TEXTURE2D_DESC tdesc = {};
    tdesc.Width            = (UINT)m_textureW;
    tdesc.Height           = (UINT)m_textureH;
    tdesc.MipLevels        = 1;
    tdesc.ArraySize        = 1;
    tdesc.Format           = DXGI_FORMAT_B8G8R8A8_UNORM;
    tdesc.SampleDesc.Count = 1;
    tdesc.Usage            = D3D11_USAGE_DEFAULT;
    tdesc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

    if (FAILED(m_d3d11Device->CreateTexture2D(&tdesc, nullptr, &m_captureTexture)) || !m_captureTexture)
    {
        m_captureTexture = nullptr;
        return false;
    }

    m_memDC = CreateCompatibleDC(nullptr);
    if (!m_memDC)
    {
        ReleaseCaptureResources();
        return false;
    }

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = m_bufW;
    bmi.bmiHeader.biHeight      = -m_bufH;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    m_bitmap = CreateDIBSection(m_memDC, &bmi, DIB_RGB_COLORS, &m_bits, nullptr, 0);
    if (!m_bitmap || !m_bits)
    {
        ReleaseCaptureResources();
        return false;
    }

    m_oldBitmap = SelectObject(m_memDC, m_bitmap);
    ZeroMemory(m_bits, (size_t)m_bufW * (size_t)m_bufH * 4u);

    m_forceUpload = true;
    m_lastFrameHash = 0;
    ++m_textureGeneration;  // Yeni texture: renderer cache'i gecersiz olmali

    return true;
}

void CaptureManager::ReleaseCaptureResources()
{
    if (m_memDC)
    {
        if (m_oldBitmap)
        {
            SelectObject(m_memDC, m_oldBitmap);
            m_oldBitmap = nullptr;
        }
        DeleteDC(m_memDC);
        m_memDC = nullptr;
    }
    if (m_bitmap)
    {
        DeleteObject(m_bitmap);
        m_bitmap = nullptr;
    }
    m_bits = nullptr;

    if (m_captureTexture)
    {
        // Texture kimligi degisti: renderer'in SRV cache'i gecersiz olmali.
        ++m_textureGeneration;
        SafeRelease(m_captureTexture);
    }
    m_packBuffer.clear();
    m_frameChanged = false;
}

// ----- Metrics ---------------------------------------------------------------

bool CaptureManager::UpdateClientSize()
{
    if (!IsWindow(m_targetHwnd)) return false;

    RECT rcClient = {};
    if (!GetClientRect(m_targetHwnd, &rcClient)) return false;
    int cw = rcClient.right - rcClient.left;
    int ch = rcClient.bottom - rcClient.top;
    if (cw <= 0 || ch <= 0) return false;

    RECT rcWindow = {};
    if (!GetWindowRect(m_targetHwnd, &rcWindow)) return false;
    int ww = rcWindow.right - rcWindow.left;
    int wh = rcWindow.bottom - rcWindow.top;
    if (ww <= 0 || wh <= 0) return false;

    POINT ptClient = { 0, 0 };
    ClientToScreen(m_targetHwnd, &ptClient);
    int ox = ptClient.x - rcWindow.left;
    int oy = ptClient.y - rcWindow.top;
    if (ox < 0) ox = 0;
    if (oy < 0) oy = 0;
    if (ox > ww - cw) ox = ww - cw;
    if (oy > wh - ch) oy = wh - ch;

    m_clientW = cw;
    m_clientH = ch;

    if (cw == m_textureW && ch == m_textureH && ww == m_bufW && wh == m_bufH)
    {
        m_offX = ox;
        m_offY = oy;
        return true;
    }

    m_textureW = cw;
    m_textureH = ch;
    m_bufW     = ww;
    m_bufH     = wh;
    m_offX     = ox;
    m_offY     = oy;

    ReleaseCaptureResources();
    return CreateCaptureResources();
}

// ----- Frame grab ------------------------------------------------------------

static unsigned int HashClientRegion(const BYTE* base, int x0, int y0,
                                     int w, int h, int strideBytes)
{
    const int step = 2;
    unsigned int hash = 2166136261u;
    for (int y = y0; y < y0 + h; y += step)
    {
        const unsigned int* row =
            (const unsigned int*)(base + (size_t)y * (size_t)strideBytes);
        for (int x = x0; x < x0 + w; x += step)
            hash = (hash ^ row[x]) * 16777619u;
    }
    return hash;
}

bool CaptureManager::GrabFrame()
{
    if (!m_memDC || !m_bits || !m_captureTexture || !m_d3d11Context || !IsWindow(m_targetHwnd))
        return false;

    BOOL ok = PrintWindow(m_targetHwnd, m_memDC, PW_RENDERFULLCONTENT);
    if (!ok) ok = PrintWindow(m_targetHwnd, m_memDC, 0);
    if (!ok) return false;

    GdiFlush();

    const int dibPitch = m_bufW * 4;
    const unsigned int frameHash = HashClientRegion(
        (const BYTE*)m_bits, m_offX, m_offY, m_textureW, m_textureH, dibPitch);

    if (!m_forceUpload && frameHash == m_lastFrameHash)
    {
        m_frameChanged = false;
        return true;
    }
    m_lastFrameHash = frameHash;
    m_frameChanged  = true;
    m_forceUpload   = false;

    const UINT dstPitch = (UINT)m_textureW * 4u;
    const bool tightlyPacked = (m_offX == 0 && m_offY == 0 && m_bufW == m_textureW);
    if (tightlyPacked)
    {
        m_d3d11Context->UpdateSubresource(m_captureTexture, 0, nullptr, m_bits, dstPitch, 0);
    }
    else
    {
        const UINT srcPitch = (UINT)m_bufW * 4u;
        const BYTE* srcBase  = (const BYTE*)m_bits
                             + (size_t)m_offY * srcPitch
                             + (size_t)m_offX * 4u;
        m_packBuffer.resize((size_t)dstPitch * (size_t)m_textureH);
        BYTE* dst = m_packBuffer.data();
        for (int y = 0; y < m_textureH; ++y)
            memcpy(dst + (size_t)y * dstPitch, srcBase + (size_t)y * srcPitch, dstPitch);

        m_d3d11Context->UpdateSubresource(m_captureTexture, 0, nullptr, m_packBuffer.data(), dstPitch, 0);
    }
    return true;
}

// ----- Public ----------------------------------------------------------------

bool CaptureManager::StartCapture(HWND targetHwnd, ID3D11Device* d3d11Device)
{
    StopCapture();
    if (!IsWindow(targetHwnd) || !d3d11Device) return false;

    std::lock_guard<std::mutex> lock(m_captureMutex);

    m_targetHwnd  = targetHwnd;
    m_d3d11Device = d3d11Device;
    m_d3d11Device->GetImmediateContext(&m_d3d11Context);

    if (!UpdateClientSize() || !m_d3d11Context)
    {
        m_targetHwnd  = nullptr;
        m_d3d11Device = nullptr;
        SafeRelease(m_d3d11Context);
        return false;
    }

    GrabFrame();
    return true;
}

void CaptureManager::StopCapture()
{
    std::lock_guard<std::mutex> lock(m_captureMutex);

    ReleaseCaptureResources();
    SafeRelease(m_d3d11Context);

    m_targetHwnd  = nullptr;
    m_d3d11Device = nullptr;
    m_textureW    = 0;
    m_textureH    = 0;
    m_clientW     = 0;
    m_clientH     = 0;
    m_bufW        = 0;
    m_bufH        = 0;
    m_offX        = 0;
    m_offY        = 0;

    m_lastFrameHash = 0;
    m_forceUpload   = true;
    m_frameChanged  = false;
}

void CaptureManager::GetClientCrop(float outUV[4]) const
{
    outUV[0] = 0.0f;
    outUV[1] = 0.0f;
    outUV[2] = 1.0f;
    outUV[3] = 1.0f;
}

ID3D11Texture2D* CaptureManager::GetLatestTexture()
{
    if (!m_captureTexture || !m_d3d11Device || !m_d3d11Context) return nullptr;
    if (!IsTargetAlive() || IsTargetMinimized()) return nullptr;

    if (!UpdateClientSize() || !m_captureTexture)
        return nullptr;

    if (!GrabFrame())
        return m_captureTexture;

    return m_captureTexture;
}

bool CaptureManager::IsTargetAlive() const
{
    if (!m_targetHwnd) return false;
    return IsWindow(m_targetHwnd) != FALSE;
}

bool CaptureManager::IsTargetMinimized() const
{
    if (!m_targetHwnd) return true;
    return IsIconic(m_targetHwnd) != FALSE;
}