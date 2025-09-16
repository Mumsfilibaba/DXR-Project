#include "Helpers.hlsli"
#include "Constants.hlsli"

// Debugging
#ifdef ENABLE_DEBUG
    #define DEBUG_LUMINANCE    0
    #define DEBUG_EDGES        1
    #define PASSTHROUGH        0
    #define DEBUG              0
    #define DEBUG_HORIZONTAL   0
    #define DEBUG_NEGPOS       0
    #define DEBUG_STEP         0
    #define DEBUG_BLEND_FACTOR 0
    #define DEBUG_RANGE        0
#endif

// FXAA Settings
#define FXAA_EDGE_THRESHOLD (1.0 / 8.0)
#define FXAA_EDGE_THRESHOLD_MIN (1.0 / 24.0)
#define FXAA_SUBPIX_TRIM (1.0 / 4.0)
#define FXAA_SUBPIX_CAP (3.0 / 4.0)
#define FXAA_SUBPIX_TRIM_SCALE (1.0 / (1.0 - FXAA_SUBPIX_TRIM))
#define FXAA_SEARCH_THRESHOLD (1.0 / 4.0)
#define FXAA_SEARCH_STEPS 24

SHADER_CONSTANT_BLOCK_BEGIN
    float2 TextureSize;
SHADER_CONSTANT_BLOCK_END

Texture2D FinalImage : register(t0);
SamplerState Sampler : register(s0);

float4 FXAASample(in Texture2D Texture, in SamplerState InSampler, float2 TexCoord)
{
    return Texture.SampleLevel(InSampler, TexCoord, 0.0);
}

float4 FXAASampleOffset(in Texture2D Texture, in SamplerState InSampler, float2 TexCoord, int2 Offset)
{
    return Texture.SampleLevel(InSampler, TexCoord, 0.0, Offset);
}

float4 FXAASampleGrad(in Texture2D Texture, in SamplerState InSampler, float2 TexCoord, float2 Grad)
{
    return Texture.SampleGrad(InSampler, TexCoord, Grad, Grad);
}

float4 Main(float2 TexCoord : TEXCOORD0) : SV_TARGET0
{
    uint Width;
    uint Height;
    FinalImage.GetDimensions(Width, Height);

    const float2 InvTextureSize = 1.0 / float2(Width, Height); //TextureSize;
    
    float4 M = FXAASampleOffset(FinalImage, Sampler, TexCoord, int2(0, 0));
    float LumaM = M.a;
#if PASSTHROUGH
    return M;
#endif
    
#if DEBUG_LUMINANCE
    return ToFloat4(M.a);
#endif
    
    float4 N = FXAASampleOffset(FinalImage, Sampler, TexCoord, int2( 0, -1));
    float4 S = FXAASampleOffset(FinalImage, Sampler, TexCoord, int2( 0,  1));
    float4 W = FXAASampleOffset(FinalImage, Sampler, TexCoord, int2(-1,  0));
    float4 E = FXAASampleOffset(FinalImage, Sampler, TexCoord, int2( 1,  0));
    float LumaN = N.a;
    float LumaS = S.a;
    float LumaW = W.a;
    float LumaE = E.a;
    
    float RangeMin = min(LumaM, min(min(LumaN, LumaS), min(LumaW, LumaE)));
    float RangeMax = max(LumaM, max(max(LumaN, LumaS), max(LumaW, LumaE)));
    float Range    = RangeMax - RangeMin;
    if (Range < max(FXAA_EDGE_THRESHOLD_MIN, RangeMax * FXAA_EDGE_THRESHOLD))
    {
#if DEBUG
        return float4(ToFloat3(M.a), 1.0);
#else
        return float4(M.rgb, 1.0);
#endif
    }
    
#if DEBUG_EDGES
    return float4(1.0, 0.0, 0.0, 1.0);
#endif
    
    float LumaL  = (LumaN + LumaS + LumaW + LumaE) * 0.25;
    float RangeL = abs(LumaL - LumaM);
    float BlendL = max(0.0, (RangeL / Range) - FXAA_SUBPIX_TRIM) * FXAA_SUBPIX_TRIM_SCALE;
    BlendL       = min(BlendL, FXAA_SUBPIX_CAP);
    
#if DEBUG_RANGE
    return ToFloat4(RangeL);
#endif
    
#if DEBUG_BLEND_FACTOR
    return ToFloat4(BlendL);
#endif
    
    float4 NW = FXAASampleOffset(FinalImage, Sampler, TexCoord, int2(-1, -1));
    float4 SW = FXAASampleOffset(FinalImage, Sampler, TexCoord, int2(-1,  1));
    float4 NE = FXAASampleOffset(FinalImage, Sampler, TexCoord, int2( 1, -1));
    float4 SE = FXAASampleOffset(FinalImage, Sampler, TexCoord, int2( 1,  1));
    float LumaNW = NW.a;
    float LumaNE = NE.a;
    float LumaSW = SW.a;
    float LumaSE = SE.a;
    
    float3 ColorL = (M.rgb + N.rgb + S.rgb + W.rgb + E.rgb) + (NW.rgb + SW.rgb + NE.rgb + SE.rgb);
    ColorL        = ColorL * 1.0 / 9.0;
    
    float EdgeVert =
        abs((0.25 * LumaNW) + (-0.5 * LumaN) + (0.25 * LumaNE)) +
        abs((0.50 * LumaW)  + (-1.0 * LumaM) + (0.50 * LumaE)) +
        abs((0.25 * LumaSW) + (-0.5 * LumaS) + (0.25 * LumaSE));
    float EdgeHorz =
        abs((0.25 * LumaNW) + (-0.5 * LumaW) + (0.25 * LumaSW)) +
        abs((0.50 * LumaN)  + (-1.0 * LumaM) + (0.50 * LumaS)) +
        abs((0.25 * LumaNE) + (-0.5 * LumaE) + (0.25 * LumaSE));
    
    bool bIsHorizontal = (EdgeHorz >= EdgeVert);
#if DEBUG_HORIZONTAL
    if (bIsHorizontal)
    {
        return float4(0.0, 1.0, 0.0, 1.0);
    }
    else
    {
        return float4(0.0, 0.0, 1.0, 1.0);
    }
#endif
    
    if (!bIsHorizontal)
    {
        LumaN = LumaW;
    }
    if (!bIsHorizontal)
    {
        LumaS = LumaE;
    }
    
    float Gradient0 = abs(LumaN - LumaM);
    float Gradient1 = abs(LumaS - LumaM);
    float LumaAvg0  = (LumaN + LumaM) * 0.5;
    float LumaAvg1  = (LumaS + LumaM) * 0.5;
    
    bool  bPair0         = (Gradient0 >= Gradient1);
    float LocalLumaAvg  = (!bPair0) ? LumaAvg1 : LumaAvg0;
    float LocalGradient = (!bPair0) ? Gradient1 : Gradient0;
    LocalGradient = LocalGradient * FXAA_SEARCH_THRESHOLD;

    float StepLength = bIsHorizontal ? -InvTextureSize.y : -InvTextureSize.x;
    if (!bPair0)
    {
        StepLength *= -1.0;
    }
    
    float2 CurrentTexCoord = TexCoord;
    if (bIsHorizontal)
    {
        CurrentTexCoord.y += StepLength * 0.5;
    }
    else
    {
        CurrentTexCoord.x += StepLength * 0.5;
    }
    
    float2 Offset    = bIsHorizontal ? float2(InvTextureSize.x, 0.0) : float2(0.0, InvTextureSize.y);
    bool   bDone0    = false;
    bool   bDone1    = false;
    float  LumaEnd0  = LocalLumaAvg;
    float  LumaEnd1  = LocalLumaAvg;
    float2 TexCoord0 = CurrentTexCoord - Offset;
    float2 TexCoord1 = CurrentTexCoord + Offset;
    
    for (int Steps = 0; Steps < FXAA_SEARCH_STEPS; Steps++)
    {
        if (!bDone0)
        {
            float4 Sample = FXAASample(FinalImage, Sampler, TexCoord0);
            LumaEnd0 = Sample.a;
        }
        if(!bDone1)
        {
            float4 Sample = FXAASample(FinalImage, Sampler, TexCoord1);
            LumaEnd1 = Sample.a;
        }
        
        bDone0 = (abs(LumaEnd0 - LocalLumaAvg) >= LocalGradient);
        bDone1 = (abs(LumaEnd1 - LocalLumaAvg) >= LocalGradient);
        if (bDone0 && bDone1)
        {
            break;
        }
        
        if (!bDone0)
        {
            TexCoord0 -= Offset;
        }
        if (!bDone1)
        {
            TexCoord1 += Offset;
        }
    }
    
    float Distance0 = bIsHorizontal ? (TexCoord.x - TexCoord0.x) : (TexCoord.y - TexCoord0.y);
    float Distance1 = bIsHorizontal ? (TexCoord1.x - TexCoord.x) : (TexCoord1.y - TexCoord.y);
    
    bool bDir0 = Distance0 < Distance1;
#if DEBUG_NEGPOS
    if(bDir0)
    {
        return float4(1.0, 0.0, 0.0, 1.0);
    }
    else
    {
        return float4(0.0, 0.0, 1.0, 1.0);
    }
#endif

    LumaEnd0 = bDir0 ? LumaEnd0 : LumaEnd1;
    if (((LumaM - LocalLumaAvg) < 0.0) == ((LumaEnd0 - LocalLumaAvg) < 0.0))
    {
        StepLength = 0.0;
    }
    
#if DEBUG_STEP
    return ToFloat4(StepLength);
#endif
    
    float SpanLength = (Distance0 + Distance1);
    Distance0        = bDir0 ? Distance0 : Distance1;
    
#if DEBUG_STEP
    return ToFloat4(StepLength);
#endif
    
    float  SubPixelOffset = (0.5 + (Distance0 * (-1.0 / SpanLength))) * StepLength;
    float2 FinalTexCoord  = TexCoord + float2(bIsHorizontal ? 0.0 : SubPixelOffset, bIsHorizontal ? SubPixelOffset : 0.0);
    float3 ColorF         = FXAASample(FinalImage, Sampler, FinalTexCoord).rgb;
    float3 FinalColor     = Lerp(ColorL, ColorF, BlendL);
    
    return float4(FinalColor, 1.0);
}