// UniversalBorderlessUpscaler (UBU) - Direct3D 11 Shaders
Texture2D g_Texture : register(t0);
SamplerState g_LinearSampler : register(s0);
SamplerState g_PointSampler  : register(s1);

cbuffer ShaderParams : register(b0)
{
    float2 g_SrcSize;       // Game client width, height (e.g., 800, 600)
    float2 g_InvSrcSize;    // 1 / 800, 1 / 600
    float2 g_DstSize;       // Monitor width, height (e.g., 1366, 768)
    float2 g_InvDstSize;    // 1 / 1366, 1 / 768
    float2 g_UVMin;         // UV crop min (crops title bar and borders)
    float2 g_UVMax;         // UV crop max
    float  g_Sharpness;     // 0.0 to 1.0
    int    g_Mode;          // 1: CAS, 2: FSR, 3: INTEGER
    float2 g_Padding;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 UV  : TEXCOORD0;
};

// Procedural Fullscreen Triangle
// UV is generated directly from SV_VertexID so it is ALWAYS the standard
// (0,0)..(1,1) range, regardless of the constant buffer.
VSOutput VSMain(uint vertexId : SV_VertexID)
{
    VSOutput output;
    float2 baseUV = float2((vertexId << 1) & 2, vertexId & 2);
    output.Pos = float4(baseUV * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);

    // Optional UV crop (title bar / borders). Safety: if the constant buffer was not
    // supplied (values are 0) or holds a degenerate range, fall back to the full
    // texture so the shader can never collapse to sampling a single texel.
    float2 uvMin = g_UVMin;
    float2 uvMax = g_UVMax;
    if (uvMax.x <= uvMin.x || uvMax.y <= uvMin.y)
    {
        uvMin = float2(0.0f, 0.0f);
        uvMax = float2(1.0f, 1.0f);
    }

    output.UV = lerp(uvMin, uvMax, baseUV);
    return output;
}

// Mode 0: OFF - Pure hardware bilinear upscale, absolute minimum GPU cost.
// A single texture fetch; the GPU's texture unit does the filtering for free.
// This is the cheapest possible path and exists purely so users can disable
// sharpening entirely without losing the borderless fullscreen feature.
float4 PS_Off(VSOutput input) : SV_TARGET
{
    return float4(g_Texture.SampleLevel(g_LinearSampler, input.UV, 0).rgb, 1.0);
}

// Mode 1: Ultra-light CAS (DEFAULT)
// Single-pass, one-line sharpening on top of hardware bilinear filtering: just 5 taps
// and no divisions, so the Intel UHD GPU costs almost nothing here and leaves its
// budget to the game. (The old multi-tap adaptive CAS and the heavy 9-tap FSR are
// intentionally not used by default.)
float4 PS_CAS(VSOutput input) : SV_TARGET
{
    float2 uv = input.UV;
    float2 d  = g_InvSrcSize;

    float3 c = g_Texture.SampleLevel(g_LinearSampler, uv, 0).rgb;
    float3 n = g_Texture.SampleLevel(g_LinearSampler, uv + float2(0.0, -d.y), 0).rgb
             + g_Texture.SampleLevel(g_LinearSampler, uv + float2(0.0,  d.y), 0).rgb
             + g_Texture.SampleLevel(g_LinearSampler, uv + float2(-d.x, 0.0), 0).rgb
             + g_Texture.SampleLevel(g_LinearSampler, uv + float2( d.x, 0.0), 0).rgb;

    float k = lerp(0.10, 0.35, saturate(g_Sharpness));
    return float4(saturate(c + (c - n * 0.25) * k), 1.0);
}

// Mode 2: AMD FidelityFX FSR 1.0 (EASU + RCAS)
float4 PS_FSR(VSOutput input) : SV_TARGET
{
    float2 uv = input.UV;
    float2 d = g_InvSrcSize;

    float3 c  = g_Texture.SampleLevel(g_LinearSampler, uv, 0).rgb;
    float3 t  = g_Texture.SampleLevel(g_LinearSampler, uv + float2(0.0, -d.y), 0).rgb;
    float3 b  = g_Texture.SampleLevel(g_LinearSampler, uv + float2(0.0,  d.y), 0).rgb;
    float3 l  = g_Texture.SampleLevel(g_LinearSampler, uv + float2(-d.x, 0.0), 0).rgb;
    float3 r  = g_Texture.SampleLevel(g_LinearSampler, uv + float2( d.x, 0.0), 0).rgb;

    float3 tl = g_Texture.SampleLevel(g_LinearSampler, uv + float2(-d.x, -d.y), 0).rgb;
    float3 tr = g_Texture.SampleLevel(g_LinearSampler, uv + float2( d.x, -d.y), 0).rgb;
    float3 bl = g_Texture.SampleLevel(g_LinearSampler, uv + float2(-d.x,  d.y), 0).rgb;
    float3 br = g_Texture.SampleLevel(g_LinearSampler, uv + float2( d.x,  d.y), 0).rgb;

    float3 dH = abs(r - l);
    float3 dV = abs(b - t);
    float3 dD1 = abs(br - tl);
    float3 dD2 = abs(bl - tr);

    float lumaH = dot(dH, float3(0.299, 0.587, 0.114));
    float lumaV = dot(dV, float3(0.299, 0.587, 0.114));
    float lumaD1 = dot(dD1, float3(0.299, 0.587, 0.114));
    float lumaD2 = dot(dD2, float3(0.299, 0.587, 0.114));

    float weightH = 1.0 / (lumaH + 0.001);
    float weightV = 1.0 / (lumaV + 0.001);
    float weightD1 = 1.0 / (lumaD1 + 0.001);
    float weightD2 = 1.0 / (lumaD2 + 0.001);

    float3 easuColor = (c * 2.0 + (l + r) * weightH * 0.5 + (t + b) * weightV * 0.5 + (tl + br) * weightD1 * 0.25 + (tr + bl) * weightD2 * 0.25) /
                       (2.0 + weightH + weightV + weightD1 * 0.5 + weightD2 * 0.5);

    float3 minC = min(c, min(min(t, b), min(l, r)));
    float3 maxC = max(c, max(max(t, b), max(l, r)));
    float3 amp = saturate(min(minC, 2.0 - maxC) / max(maxC, 0.0001));

    float rcasPeak = -lerp(0.08, 0.24, saturate(g_Sharpness));
    float3 rcasW = amp * rcasPeak;

    float3 finalColor = ( (t + b + l + r) * rcasW + easuColor ) / (4.0 * rcasW + 1.0);
    return float4(saturate(finalColor), 1.0);
}

// Mode 3: Pixel-Art Crisp Integer Scaling
float4 PS_Integer(VSOutput input) : SV_TARGET
{
    float2 uv = input.UV;
    float2 pixel = uv * g_SrcSize;
    float2 fracCoord = frac(pixel);
    float2 pixelCoord = floor(pixel);

    float2 fw = max(fwidth(pixel), float2(0.00001, 0.00001));
    float2 clampedFrac = saturate((fracCoord - 0.5) / fw + 0.5);
    float2 sampleUV = (pixelCoord + clampedFrac) * g_InvSrcSize;

    float3 color = g_Texture.SampleLevel(g_LinearSampler, sampleUV, 0).rgb;
    return float4(color, 1.0);
}