#include "D3D11Renderer.h"
#include "ShaderSources.h"
#include <fstream>
#include <sstream>
#include <cstring>

D3D11Renderer::D3D11Renderer()
{
}

D3D11Renderer::~D3D11Renderer()
{
    Cleanup();
}

bool D3D11Renderer::Initialize(HWND overlayHwnd, int width, int height)
{
    Cleanup();

    m_hwnd = overlayHwnd;
    m_outputWidth = width;
    m_outputHeight = height;

    if (!CreateDeviceAndSwapChain(overlayHwnd, width, height))
    {
        Cleanup();
        return false;
    }

    if (!CreateRenderTarget())
    {
        Cleanup();
        return false;
    }

    if (!CreateShaders())
    {
        Cleanup();
        return false;
    }

    if (!CreateSamplers())
    {
        Cleanup();
        return false;
    }

    if (!CreateConstantBuffer())
    {
        Cleanup();
        return false;
    }

    if (!CreateCursorResources())
    {
        Cleanup();
        return false;
    }

    m_deviceLost = false;
    return true;
}

void D3D11Renderer::Cleanup()
{
    SafeRelease(m_currentSRV);
    m_lastBoundTexture = nullptr;

    ReleaseCursorResources();

    SafeRelease(m_linearSampler);
    SafeRelease(m_pointSampler);
    SafeRelease(m_constantBuffer);

    SafeRelease(m_vs);
    SafeRelease(m_psOff);
    SafeRelease(m_psCAS);
    SafeRelease(m_psFSR);
    SafeRelease(m_psInteger);

    ReleaseRenderTarget();

    if (m_frameLatencyWaitableObject)
    {
        CloseHandle(m_frameLatencyWaitableObject);
        m_frameLatencyWaitableObject = nullptr;
    }

    SafeRelease(m_swapChain2);
    SafeRelease(m_swapChain);
    SafeRelease(m_context);
    SafeRelease(m_device);

    // Durumu sifirla (RecoverDevice tutarliligi icin)
    m_allowTearing  = false;
    m_swapChainFlags = 0;
    m_outputWidth   = 0;
    m_outputHeight  = 0;
    m_hwnd          = nullptr;
}

bool D3D11Renderer::CreateDeviceAndSwapChain(HWND overlayHwnd, int width, int height)
{
    UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };
    D3D_FEATURE_LEVEL featureLevel;

    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createDeviceFlags,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &m_device,
        &featureLevel,
        &m_context
    );

    if (FAILED(hr))
    {
        hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            nullptr,
            0,
            D3D11_SDK_VERSION,
            &m_device,
            &featureLevel,
            &m_context
        );
        if (FAILED(hr)) return false;
    }

    IDXGIDevice1* dxgiDevice = nullptr;
    hr = m_device->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
    if (FAILED(hr)) return false;

    dxgiDevice->SetMaximumFrameLatency(1);

    IDXGIAdapter* dxgiAdapter = nullptr;
    hr = dxgiDevice->GetAdapter(&dxgiAdapter);
    dxgiDevice->Release();
    if (FAILED(hr)) return false;

    IDXGIFactory2* dxgiFactory2 = nullptr;
    hr = dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory2));
    dxgiAdapter->Release();
    if (FAILED(hr)) return false;

    DXGI_SWAP_CHAIN_DESC1 scDesc = {};
    scDesc.Width = width;
    scDesc.Height = height;
    scDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    scDesc.Stereo = FALSE;
    scDesc.SampleDesc.Count = 1;
    scDesc.SampleDesc.Quality = 0;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.BufferCount = 2;
    scDesc.Scaling = DXGI_SCALING_NONE;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    // Preferred path: ALLOW_TEARING -> Present() with zero wait, no vsync blocking.
    // This lets UBU output frames as fast as the game produces them without the
    // compositor waiting, which is the lowest-latency option on Intel UHD.
    m_allowTearing = false;
    scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    hr = dxgiFactory2->CreateSwapChainForHwnd(
        m_device,
        overlayHwnd,
        &scDesc,
        nullptr,
        nullptr,
        &m_swapChain
    );

    if (SUCCEEDED(hr))
    {
        m_allowTearing = true;
        m_swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }
    else
    {
        // Fallback 1: frame-latency waitable object (vsync-paced, 1-frame wait).
        scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        hr = dxgiFactory2->CreateSwapChainForHwnd(
            m_device,
            overlayHwnd,
            &scDesc,
            nullptr,
            nullptr,
            &m_swapChain
        );

        if (SUCCEEDED(hr))
        {
            m_swapChainFlags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        }
        else
        {
            // Fallback 2: plain swap chain.
            scDesc.Flags = 0;
            hr = dxgiFactory2->CreateSwapChainForHwnd(
                m_device,
                overlayHwnd,
                &scDesc,
                nullptr,
                nullptr,
                &m_swapChain
            );
            if (FAILED(hr))
            {
                dxgiFactory2->Release();
                return false;
            }
            m_swapChainFlags = 0;
        }
    }

    dxgiFactory2->MakeWindowAssociation(overlayHwnd, DXGI_MWA_NO_ALT_ENTER);
    dxgiFactory2->Release();

    if (SUCCEEDED(m_swapChain->QueryInterface(IID_PPV_ARGS(&m_swapChain2))))
    {
        m_swapChain2->SetMaximumFrameLatency(1);
        if (!m_allowTearing)
            m_frameLatencyWaitableObject = m_swapChain2->GetFrameLatencyWaitableObject();
    }

    return true;
}

bool D3D11Renderer::CreateRenderTarget()
{
    ID3D11Texture2D* backBuffer = nullptr;
    HRESULT hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr)) return false;

    hr = m_device->CreateRenderTargetView(backBuffer, nullptr, &m_backBufferRTV);
    backBuffer->Release();
    return SUCCEEDED(hr);
}

void D3D11Renderer::ReleaseRenderTarget()
{
    SafeRelease(m_backBufferRTV);
}

bool D3D11Renderer::Resize(int width, int height)
{
    if (!m_swapChain) return false;

    ReleaseRenderTarget();
    m_outputWidth = width;
    m_outputHeight = height;

    // ResizeBuffers, swapchain'in ILK olusturuldugu bayraklarla birebir cagrilmalidir.
    // Aksi halde (or. ALLOW_TEARING dusurulur/eklenirse) DXGI E_INVALIDARG dondurur
    // ve yeniden boyutlandirma (tam ekrana gecis) tamamen basarisiz olur.
    HRESULT hr = m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, m_swapChainFlags);

    if (FAILED(hr))
    {
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            m_deviceLost = true;
        }
        return false;
    }

    return CreateRenderTarget();
}

static ID3DBlob* CompileHLSL(const char* sourceCode, size_t length, const char* entryPoint, const char* target)
{
    ID3DBlob* blob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;

    HRESULT hr = D3DCompile(
        sourceCode,
        length,
        nullptr,
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPoint,
        target,
        flags,
        0,
        &blob,
        &errorBlob
    );

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }
        return nullptr;
    }

    if (errorBlob) errorBlob->Release();
    return blob;
}

// HLSL kaynagini UBU.exe'nin bulundugu klasore GORE arar; boylece program
// hangi klasorden baslatilirsa baslatilsin ayni (dogru) shader yuklenir.
// Yol hesaplanamazsa veya dosya yoksa gomulu ShaderSources::MainHLSL kullanilir.
static bool LoadShaderFileFromExeDir(std::string& outCode)
{
    wchar_t exePath[MAX_PATH] = {};
    if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) == 0)
        return false;

    std::wstring dir(exePath);
    const size_t slash = dir.find_last_of(L"\\/");
    if (slash == std::wstring::npos)
        return false;
    dir.resize(slash + 1);

    const wchar_t* candidates[] = {
        L"src\\Shaders\\MainShaders.hlsl",   // depo (repo) duzeni
        L"Shaders\\MainShaders.hlsl"         // dagitim (release) duzeni
    };

    for (const wchar_t* rel : candidates)
    {
        std::wstring full = dir + rel;
        std::ifstream file(full.c_str(), std::ios::binary);
        if (!file.is_open())
            continue;

        std::stringstream ss;
        ss << file.rdbuf();
        outCode = ss.str();
        if (outCode.empty())
            continue;

        // UTF-8 BOM varsa D3DCompile bunu kabul etmez -> temizle
        if (outCode.size() >= 3 &&
            (unsigned char)outCode[0] == 0xEF &&
            (unsigned char)outCode[1] == 0xBB &&
            (unsigned char)outCode[2] == 0xBF)
        {
            outCode.erase(0, 3);
        }
        return true;
    }
    return false;
}

bool D3D11Renderer::CreateShaders()
{
    std::string shaderCode;
    if (!LoadShaderFileFromExeDir(shaderCode))
        shaderCode = ShaderSources::MainHLSL;

    ID3DBlob* vsBlob = CompileHLSL(shaderCode.data(), shaderCode.size(), "VSMain", "vs_5_0");
    if (!vsBlob) return false;
    HRESULT hr = m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vs);
    vsBlob->Release();
    if (FAILED(hr)) return false;

    ID3DBlob* offBlob = CompileHLSL(shaderCode.data(), shaderCode.size(), "PS_Off", "ps_5_0");
    if (!offBlob) return false;
    hr = m_device->CreatePixelShader(offBlob->GetBufferPointer(), offBlob->GetBufferSize(), nullptr, &m_psOff);
    offBlob->Release();
    if (FAILED(hr)) return false;

    ID3DBlob* casBlob = CompileHLSL(shaderCode.data(), shaderCode.size(), "PS_CAS", "ps_5_0");
    if (!casBlob) return false;
    hr = m_device->CreatePixelShader(casBlob->GetBufferPointer(), casBlob->GetBufferSize(), nullptr, &m_psCAS);
    casBlob->Release();
    if (FAILED(hr)) return false;

    ID3DBlob* fsrBlob = CompileHLSL(shaderCode.data(), shaderCode.size(), "PS_FSR", "ps_5_0");
    if (!fsrBlob) return false;
    hr = m_device->CreatePixelShader(fsrBlob->GetBufferPointer(), fsrBlob->GetBufferSize(), nullptr, &m_psFSR);
    fsrBlob->Release();
    if (FAILED(hr)) return false;

    ID3DBlob* intBlob = CompileHLSL(shaderCode.data(), shaderCode.size(), "PS_Integer", "ps_5_0");
    if (!intBlob) return false;
    hr = m_device->CreatePixelShader(intBlob->GetBufferPointer(), intBlob->GetBufferSize(), nullptr, &m_psInteger);
    intBlob->Release();
    if (FAILED(hr)) return false;

    return true;
}

bool D3D11Renderer::CreateSamplers()
{
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

    HRESULT hr = m_device->CreateSamplerState(&sampDesc, &m_linearSampler);
    if (FAILED(hr)) return false;

    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    hr = m_device->CreateSamplerState(&sampDesc, &m_pointSampler);
    return SUCCEEDED(hr);
}

bool D3D11Renderer::CreateConstantBuffer()
{
    D3D11_BUFFER_DESC bDesc = {};
    bDesc.ByteWidth = sizeof(ShaderConstants);
    bDesc.Usage = D3D11_USAGE_DYNAMIC;
    bDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = m_device->CreateBuffer(&bDesc, nullptr, &m_constantBuffer);
    return SUCCEEDED(hr);
}

// -----------------------------------------------------------------------------
// Gercek Windows sistem ok imlecini (IDC_ARROW) DPI/tema boyutuna uygun
// sekilde 32-bit premultiplied BGRA piksel dizisine yakalar.
// Siyah + beyaz arka plana cizim ile anti-aliasing ve golge alfa degerleri
// birebir kurtarilir.
// -----------------------------------------------------------------------------
static bool CaptureSystemArrowCursor(std::vector<uint32_t>& outPixels,
                                     int& outW, int& outH,
                                     int& outHotX, int& outHotY)
{
    HCURSOR hCur = (HCURSOR)LoadImageW(
        nullptr, MAKEINTRESOURCEW(32512), IMAGE_CURSOR, 0, 0, LR_SHARED | LR_DEFAULTSIZE);
    if (!hCur) return false;

    ICONINFO ii = {};
    if (!GetIconInfo(hCur, &ii)) return false;

    int w = 0, h = 0;
    BITMAP bm = {};
    if (ii.hbmColor && GetObjectW(ii.hbmColor, sizeof(bm), &bm))
    {
        w = bm.bmWidth;
        h = std::abs(bm.bmHeight);
    }
    else if (ii.hbmMask && GetObjectW(ii.hbmMask, sizeof(bm), &bm))
    {
        w = bm.bmWidth;
        h = std::abs(bm.bmHeight) / 2;
    }

    const int hotX = (int)ii.xHotspot;
    const int hotY = (int)ii.yHotspot;
    if (ii.hbmColor) DeleteObject(ii.hbmColor);
    if (ii.hbmMask)  DeleteObject(ii.hbmMask);

    if (w <= 0 || h <= 0 || w > 256 || h > 256)
        return false;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = w;
    bmi.bmiHeader.biHeight      = -h;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC hdcScreen = GetDC(nullptr);
    HDC dcBlack   = CreateCompatibleDC(hdcScreen);
    HDC dcWhite   = CreateCompatibleDC(hdcScreen);

    void* bitsBlack = nullptr;
    void* bitsWhite = nullptr;
    HBITMAP bmpBlack = CreateDIBSection(dcBlack, &bmi, DIB_RGB_COLORS, &bitsBlack, nullptr, 0);
    HBITMAP bmpWhite = CreateDIBSection(dcWhite, &bmi, DIB_RGB_COLORS, &bitsWhite, nullptr, 0);

    bool ok = false;
    if (dcBlack && dcWhite && bmpBlack && bmpWhite && bitsBlack && bitsWhite)
    {
        HGDIOBJ oldB = SelectObject(dcBlack, bmpBlack);
        HGDIOBJ oldW = SelectObject(dcWhite, bmpWhite);

        const size_t count = (size_t)w * (size_t)h;
        uint32_t* pxB = (uint32_t*)bitsBlack;
        uint32_t* pxW = (uint32_t*)bitsWhite;
        for (size_t i = 0; i < count; ++i)
        {
            pxB[i] = 0x00000000u;
            pxW[i] = 0x00FFFFFFu;
        }

        const BOOL d1 = DrawIconEx(dcBlack, 0, 0, hCur, w, h, 0, nullptr, DI_NORMAL);
        const BOOL d2 = DrawIconEx(dcWhite, 0, 0, hCur, w, h, 0, nullptr, DI_NORMAL);
        GdiFlush();

        if (d1 && d2)
        {
            outPixels.resize(count);
            bool anyOpaque = false;
            for (size_t i = 0; i < count; ++i)
            {
                const uint32_t bPx = pxB[i];
                const uint32_t wPx = pxW[i];
                const int bB = (int)(bPx & 0xFFu);
                const int bG = (int)((bPx >> 8) & 0xFFu);
                const int bR = (int)((bPx >> 16) & 0xFFu);
                const int wB = (int)(wPx & 0xFFu);
                const int wG = (int)((wPx >> 8) & 0xFFu);
                const int wR = (int)((wPx >> 16) & 0xFFu);

                const int diff = ((wB - bB) + (wG - bG) + (wR - bR) + 1) / 3;
                const int invA = std::clamp(diff, 0, 255);
                const int a    = 255 - invA;
                if (a == 0)
                {
                    outPixels[i] = 0u;
                }
                else
                {
                    anyOpaque = true;
                    const uint32_t r = (uint32_t)std::clamp(bR, 0, a);
                    const uint32_t g = (uint32_t)std::clamp(bG, 0, a);
                    const uint32_t b = (uint32_t)std::clamp(bB, 0, a);
                    outPixels[i] = ((uint32_t)a << 24) | (r << 16) | (g << 8) | b;
                }
            }
            if (anyOpaque)
            {
                outW = w;
                outH = h;
                outHotX = std::clamp(hotX, 0, w - 1);
                outHotY = std::clamp(hotY, 0, h - 1);
                ok = true;
            }
        }

        SelectObject(dcBlack, oldB);
        SelectObject(dcWhite, oldW);
    }

    if (bmpBlack) DeleteObject(bmpBlack);
    if (bmpWhite) DeleteObject(bmpWhite);
    if (dcBlack)  DeleteDC(dcBlack);
    if (dcWhite)  DeleteDC(dcWhite);
    if (hdcScreen) ReleaseDC(nullptr, hdcScreen);

    return ok;
}

// -----------------------------------------------------------------------------
// Sentetik fare imleci kaynaklari.
// Oncelik: gercek Windows sistem ok imleci (birebir gorunum, anti-aliasing,
// golge ve DPI boyutu). Basarisiz olursa prosedurel klasik ok kullanilir.
// -----------------------------------------------------------------------------
bool D3D11Renderer::CreateCursorResources()
{
    std::vector<uint32_t> pixels;
    if (!CaptureSystemArrowCursor(pixels, m_cursorTexW, m_cursorTexH, m_cursorHotX, m_cursorHotY))
    {
        m_cursorTexW = 16;
        m_cursorTexH = 22;
        m_cursorHotX = 2;
        m_cursorHotY = 2;

        const int W = m_cursorTexW;
        const int H = m_cursorTexH;

        static const int P[7][2] = {
            {  2,  2 }, {  2, 17 }, {  6, 13 }, {  9, 19 },
            { 11, 18 }, {  8, 12 }, { 13, 12 }
        };
        const int N = 7;

        auto Inside = [&](int x, int y) -> bool
        {
            bool in = false;
            for (int i = 0, j = N - 1; i < N; j = i++)
            {
                const int xi = P[i][0], yi = P[i][1];
                const int xj = P[j][0], yj = P[j][1];
                if (((yi > y) != (yj > y)) &&
                    (x < (double)(xj - xi) * (double)(y - yi) / (double)(yj - yi) + (double)xi))
                {
                    in = !in;
                }
            }
            return in;
        };

        pixels.assign((size_t)W * H, 0u);
        for (int y = 0; y < H; ++y)
        {
            for (int x = 0; x < W; ++x)
            {
                const bool fill = Inside(x, y);

                bool outline = false;
                if (!fill)
                {
                    for (int dy = -1; dy <= 1 && !outline; ++dy)
                    {
                        for (int dx = -1; dx <= 1; ++dx)
                        {
                            if (Inside(x + dx, y + dy)) { outline = true; break; }
                        }
                    }
                }

                uint32_t px = 0x00000000u;
                if (fill)          px = 0xFFFFFFFFu;
                else if (outline)  px = 0xFF000000u;
                pixels[(size_t)y * W + x] = px;
            }
        }
    }

    const int W = m_cursorTexW;
    const int H = m_cursorTexH;

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = (UINT)W;
    td.Height = (UINT)H;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem = pixels.data();
    sd.SysMemPitch = (UINT)(W * 4);

    if (FAILED(m_device->CreateTexture2D(&td, &sd, &m_cursorTexture))) return false;
    if (FAILED(m_device->CreateShaderResourceView(m_cursorTexture, nullptr, &m_cursorSRV))) return false;

    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = sizeof(float) * 4 * 6;   // 6 vertex x (x,y,u,v)
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(m_device->CreateBuffer(&bd, nullptr, &m_cursorVB))) return false;

    const char* src = ShaderSources::CursorHLSL;
    const size_t len = strlen(src);

    ID3DBlob* vsBlob = CompileHLSL(src, len, "CursorVS", "vs_5_0");
    if (!vsBlob) return false;

    HRESULT hr = m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_cursorVS);
    if (SUCCEEDED(hr))
    {
        D3D11_INPUT_ELEMENT_DESC elems[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        hr = m_device->CreateInputLayout(elems, ARRAYSIZE(elems),
                                         vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                                         &m_cursorInputLayout);
    }
    vsBlob->Release();
    if (FAILED(hr)) return false;

    ID3DBlob* psBlob = CompileHLSL(src, len, "CursorPS", "ps_5_0");
    if (!psBlob) return false;
    hr = m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_cursorPS);
    psBlob->Release();
    if (FAILED(hr)) return false;

    D3D11_BLEND_DESC bld = {};
    bld.RenderTarget[0].BlendEnable = TRUE;
    bld.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    bld.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    bld.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bld.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bld.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    bld.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bld.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    if (FAILED(m_device->CreateBlendState(&bld, &m_alphaBlend))) return false;

    return true;
}

void D3D11Renderer::ReleaseCursorResources()
{
    SafeRelease(m_alphaBlend);
    SafeRelease(m_cursorInputLayout);
    SafeRelease(m_cursorPS);
    SafeRelease(m_cursorVS);
    SafeRelease(m_cursorVB);
    SafeRelease(m_cursorSRV);
    SafeRelease(m_cursorTexture);
}

void D3D11Renderer::DrawCursor()
{
    if (!m_cursorTexture || !m_cursorSRV || !m_cursorVB ||
        !m_cursorVS || !m_cursorPS || !m_cursorInputLayout)
        return;

    const float outW = (float)std::max(1, m_outputWidth);
    const float outH = (float)std::max(1, m_outputHeight);

    // Dokudaki ok ucunun konumu (m_cursorHotX/Y), imlecin gercek noktasina denk gelir.
    const float x0 = m_cursorX - (float)m_cursorHotX;
    const float y0 = m_cursorY - (float)m_cursorHotY;
    const float x1 = x0 + (float)m_cursorTexW;
    const float y1 = y0 + (float)m_cursorTexH;

    auto ToNDC = [&](float px, float py, float& nx, float& ny)
    {
        nx = (px / outW) * 2.0f - 1.0f;
        ny = 1.0f - (py / outH) * 2.0f;
    };

    float nx0, ny0, nx1, ny1;
    ToNDC(x0, y0, nx0, ny0);
    ToNDC(x1, y1, nx1, ny1);

    struct CursorVertex { float x, y, u, v; };
    const CursorVertex verts[6] = {
        { nx0, ny0, 0.0f, 0.0f },
        { nx1, ny0, 1.0f, 0.0f },
        { nx0, ny1, 0.0f, 1.0f },
        { nx1, ny0, 1.0f, 0.0f },
        { nx1, ny1, 1.0f, 1.0f },
        { nx0, ny1, 0.0f, 1.0f },
    };

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (FAILED(m_context->Map(m_cursorVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        return;
    memcpy(mapped.pData, verts, sizeof(verts));
    m_context->Unmap(m_cursorVB, 0);

    // Imlec cizimi icin viewport tam ekran olmalidir (NDC hesabi buna gore).
    D3D11_VIEWPORT vp = {};
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width = outW;
    vp.Height = outH;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;

    m_context->RSSetViewports(1, &vp);
    m_context->OMSetRenderTargets(1, &m_backBufferRTV, nullptr);
    m_context->IASetInputLayout(m_cursorInputLayout);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    UINT stride = sizeof(CursorVertex);
    UINT offset = 0;
    ID3D11Buffer* vb = m_cursorVB;
    m_context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

    m_context->VSSetShader(m_cursorVS, nullptr, 0);
    m_context->PSSetShader(m_cursorPS, nullptr, 0);
    m_context->PSSetShaderResources(0, 1, &m_cursorSRV);
    m_context->PSSetSamplers(0, 1, &m_pointSampler);

    m_context->OMSetBlendState(m_alphaBlend, nullptr, 0xFFFFFFFFu);
    m_context->Draw(6, 0);
    m_context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFu);

    ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
    m_context->PSSetShaderResources(0, 1, nullSRV);
}

bool D3D11Renderer::RenderFrame(ID3D11Texture2D* capturedTexture, int srcWidth, int srcHeight, const float uvMin[2], const float uvMax[2], UpscaleMode mode, float sharpness)
{
    if (m_deviceLost || !m_swapChain || !m_backBufferRTV || !capturedTexture)
        return false;

    if (m_frameLatencyWaitableObject)
    {
        WaitForSingleObjectEx(m_frameLatencyWaitableObject, 100, TRUE);
    }

    if (m_lastBoundTexture != capturedTexture)
    {
        SafeRelease(m_currentSRV);
        m_lastBoundTexture = capturedTexture;

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        HRESULT hr = m_device->CreateShaderResourceView(capturedTexture, &srvDesc, &m_currentSRV);
        if (FAILED(hr))
        {
            hr = m_device->CreateShaderResourceView(capturedTexture, nullptr, &m_currentSRV);
            if (FAILED(hr)) return false;
        }
    }

    // Aspect Ratio Correction for Rendering
    float targetAspect = (float)srcWidth / (float)srcHeight;
    float outputAspect = (float)m_outputWidth / (float)m_outputHeight;

    float drawW = (float)m_outputWidth;
    float drawH = (float)m_outputHeight;
    float offsetX = 0.0f;
    float offsetY = 0.0f;

    // Eger hedef en-boy orani monitor ile uyumluysa (orn. 16:9), offsetX/offsetY 0 kalir.
    // Float yuvarlama farklari nedeniyle gereksiz ClearRenderTargetView tetiklenmesini onlemek icin
    // epsilon karsilastirmasi yapilir.
    if (std::abs(targetAspect - outputAspect) > 0.005f)
    {
        if (targetAspect > outputAspect)
        {
            // Hedef daha genis -> Letterbox (ust ve altta siyah serit)
            drawH = (float)m_outputWidth / targetAspect;
            offsetY = ((float)m_outputHeight - drawH) * 0.5f;
        }
        else
        {
            // Hedef daha dar -> Pillarbox (sag ve solda siyah serit)
            drawW = (float)m_outputHeight * targetAspect;
            offsetX = ((float)m_outputWidth - drawW) * 0.5f;
        }
    }

    D3D11_VIEWPORT vp = {};
    vp.TopLeftX = offsetX;
    vp.TopLeftY = offsetY;
    vp.Width = drawW;
    vp.Height = drawH;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;

    // Tam ekran ClearRenderTargetView yalnizca GERCEKTEN gerekiyorsa yapilir.
    // Normal durumda (oyun istemci alani = monitor) viewport tum ekrani
    // kapladigi icin temizleme tamamen gereksiz bant genisligi tuketimidir;
    // Intel UHD gibi paylasimli bellekli iGPU'larda bu ciddi bir yuktur.
    const bool letterboxed = (offsetX > 0.0f || offsetY > 0.0f ||
                              drawW < (float)m_outputWidth ||
                              drawH < (float)m_outputHeight);
    if (letterboxed)
    {
        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        m_context->ClearRenderTargetView(m_backBufferRTV, clearColor);
    }

    m_context->RSSetViewports(1, &vp);
    m_context->OMSetRenderTargets(1, &m_backBufferRTV, nullptr);

    // Source size safety: never let the shader receive 0 (that would make all
    // neighbour taps collapse). Fall back to the real texture descriptor.
    if (srcWidth <= 0 || srcHeight <= 0)
    {
        D3D11_TEXTURE2D_DESC texDesc = {};
        capturedTexture->GetDesc(&texDesc);
        srcWidth  = (int)texDesc.Width;
        srcHeight = (int)texDesc.Height;
    }

    // Update Constant Buffer
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(m_context->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        ShaderConstants* cb = (ShaderConstants*)mapped.pData;
        cb->srcWidth = (float)srcWidth;
        cb->srcHeight = (float)srcHeight;
        cb->invSrcWidth = 1.0f / (float)std::max(1, srcWidth);
        cb->invSrcHeight = 1.0f / (float)std::max(1, srcHeight);

        cb->dstWidth = vp.Width;
        cb->dstHeight = vp.Height;
        cb->invDstWidth = 1.0f / std::max(1.0f, vp.Width);
        cb->invDstHeight = 1.0f / std::max(1.0f, vp.Height);

        if (uvMin && uvMax)
        {
            cb->uvMin[0] = uvMin[0];
            cb->uvMin[1] = uvMin[1];
            cb->uvMax[0] = uvMax[0];
            cb->uvMax[1] = uvMax[1];
        }
        else
        {
            cb->uvMin[0] = 0.0f;
            cb->uvMin[1] = 0.0f;
            cb->uvMax[0] = 1.0f;
            cb->uvMax[1] = 1.0f;
        }

        cb->sharpness = sharpness;
        cb->mode = (int)mode;
        cb->padding[0] = 0.0f;
        cb->padding[1] = 0.0f;

        m_context->Unmap(m_constantBuffer, 0);
    }

    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->IASetInputLayout(nullptr);

    m_context->VSSetShader(m_vs, nullptr, 0);
    // The constant buffer MUST also be bound to the vertex stage: VSMain reads
    // g_UVMin / g_UVMax from b0 to build the UV coordinates. Binding it only to the
    // pixel stage left b0 unbound in the VS (all zeros), which made every vertex
    // emit UV (0,0) and stretched the top-left single pixel over the whole screen.
    m_context->VSSetConstantBuffers(0, 1, &m_constantBuffer);
    m_context->PSSetConstantBuffers(0, 1, &m_constantBuffer);

    ID3D11SamplerState* samplers[2] = { m_linearSampler, m_pointSampler };
    m_context->PSSetSamplers(0, 2, samplers);
    m_context->PSSetShaderResources(0, 1, &m_currentSRV);

    switch (mode)
    {
    case UpscaleMode::OFF:
        m_context->PSSetShader(m_psOff, nullptr, 0);
        break;
    case UpscaleMode::CAS:
        m_context->PSSetShader(m_psCAS, nullptr, 0);
        break;
    case UpscaleMode::FSR:
        m_context->PSSetShader(m_psFSR, nullptr, 0);
        break;
    case UpscaleMode::INTEGER:
        m_context->PSSetShader(m_psInteger, nullptr, 0);
        break;
    default:
        m_context->PSSetShader(m_psCAS, nullptr, 0);
        break;
    }

    m_context->Draw(3, 0);

    ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
    m_context->PSSetShaderResources(0, 1, nullSRV);

    // Olceklenmis goruntunun UZERINE sentetik fare imleci cizilir.
    if (m_cursorVisible)
        DrawCursor();

    // Zero-wait presentation when tearing is available: SyncInterval 0 +
    // DXGI_PRESENT_ALLOW_TEARING, so UBU is never blocked by the compositor.
    UINT presentFlags = m_allowTearing ? DXGI_PRESENT_ALLOW_TEARING : 0u;
    UINT syncInterval = m_allowTearing ? 0u : 1u;

    HRESULT hr = m_swapChain->Present(syncInterval, presentFlags);

    // If the runtime refuses the tearing present, fall back to a normal vsync present
    // so we never end up presenting nothing (black screen).
    if (m_allowTearing && FAILED(hr))
    {
        m_allowTearing = false;
        hr = m_swapChain->Present(1, 0);
    }

    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        m_deviceLost = true;
        return false;
    }

    return true;
}

bool D3D11Renderer::RecoverDevice(HWND overlayHwnd, int width, int height)
{
    Cleanup();
    return Initialize(overlayHwnd, width, height);
}
