#include "D3D11Renderer.h"
#include "ShaderSources.h"
#include <fstream>
#include <sstream>

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

    m_deviceLost = false;
    return true;
}

void D3D11Renderer::Cleanup()
{
    SafeRelease(m_currentSRV);
    m_lastBoundTexture = nullptr;

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

        if (FAILED(hr))
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

    UINT scFlags = 0;
    if (m_allowTearing)
        scFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    else if (m_frameLatencyWaitableObject)
        scFlags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

    HRESULT hr = m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, scFlags);

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

    if (targetAspect > outputAspect)
    {
        // Pillarbox
        drawH = (float)m_outputWidth / targetAspect;
        offsetY = ((float)m_outputHeight - drawH) * 0.5f;
    }
    else
    {
        // Letterbox
        drawW = (float)m_outputHeight * targetAspect;
        offsetX = ((float)m_outputWidth - drawW) * 0.5f;
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
