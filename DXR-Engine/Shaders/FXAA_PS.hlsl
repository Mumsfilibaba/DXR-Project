#include "PBRCommon.hlsli"

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
#define FXAA_EDGE_THRESHOLD		(1.0f / 8.0f)
#define FXAA_EDGE_THRESHOLD_MIN	(1.0f / 24.0f)
#define FXAA_SUBPIX_TRIM		(1.0f / 4.0f)
#define FXAA_SUBPIX_CAP			(3.0f / 4.0f)
#define FXAA_SUBPIX_TRIM_SCALE	(1.0f / (1.0f - FXAA_SUBPIX_TRIM))
#define FXAA_SEARCH_THRESHOLD	(1.0f / 4.0f)
#define FXAA_SEARCH_STEPS		24

cbuffer CB0 : register(b0, space0)
{
    float2 TextureSize;
}

Texture2D    FinalImage : register(t0, space0);
SamplerState Sampler    : register(s0, space0);

float4 FXAASample(in Texture2D Texture, in SamplerState InSampler, float2 TexCoord)
{
    return Texture.SampleLevel(InSampler, TexCoord, 0.0f);
}

float4 FXAASampleOffset(in Texture2D Texture, in SamplerState InSampler, float2 TexCoord, int2 Offset)
{
    return Texture.SampleLevel(InSampler, TexCoord, 0.0f, Offset);
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
    const float2 InvTextureSize = float2(1.0f, 1.0f) / float2(Width, Height); //TextureSize;
    
    float4 M = FXAASampleOffset(FinalImage, Sampler, TexCoord, int2(0, 0));
    float LumaM = M.a;
#if PASSTHROUGH
    return M;
#endif
    
#if DEBUG_LUMINANCE
    return ToFloat4(M.a);
#endif
    
    float4 N = FXAASampleOffset(FinalImage, Sampler, TexCoord, int2( 0,-1));
    float4 S = FXAASampleOffset(FinalImage, Sampler, TexCoord, int2( 0, 1));
    float4 W = FXAASampleOffset(FinalImage, Sampler, TexCoord, int2(-1, 0));
    float4 E = FXAASampleOffset(FinalImage, Sampler, TexCoord, int2( 1, 0));
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
        return float4(ToFloat3(M.a), 1.0f);
#else
        return float4(M.rgb, 1.0f);
#endif
    }
    
#if DEBUG_EDGES
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
#endif
    
    float LumaL  = (LumaN + LumaS + LumaW + LumaE) * 0.25f;
    float RangeL = abs(LumaL - LumaM);
    float BlendL = max(0.0f, (RangeL / Range) - FXAA_SUBPIX_TRIM) * FXAA_SUBPIX_TRIM_SCALE;
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
    ColorL        = ColorL * ToFloat3(1.0f / 9.0f);
    
    float EdgeVert =
        abs((0.25f * LumaNW) + (-0.5f * LumaN) + (0.25f * LumaNE)) +
        abs((0.50f * LumaW)  + (-1.0f * LumaM) + (0.50f * LumaE)) +
        abs((0.25f * LumaSW) + (-0.5f * LumaS) + (0.25f * LumaSE));
    float EdgeHorz =
        abs((0.25f * LumaNW) + (-0.5f * LumaW) + (0.25f * LumaSW)) +
        abs((0.50f * LumaN)  + (-1.0f * LumaM) + (0.50f * LumaS)) +
        abs((0.25f * LumaNE) + (-0.5f * LumaE) + (0.25f * LumaSE));
    
    bool IsHorizontal = (EdgeHorz >= EdgeVert);
#if DEBUG_HORIZONTAL
    if (IsHorizontal)
    {
        return float4(0.0f, 1.0f, 0.0f, 1.0f);
    }
    else
    {
        return float4(0.0f, 0.0f, 1.0f, 1.0f);
    }
#endif
    
    if (!IsHorizontal)
    {
        LumaN = LumaW;
    }
    if (!IsHorizontal)
    {
        LumaS = LumaE;
    }
    
    float Gradient0 = abs(LumaN - LumaM);
    float Gradient1 = abs(LumaS - LumaM);
    float LumaAvg0  = (LumaN + LumaM) * 0.5f;
    float LumaAvg1  = (LumaS + LumaM) * 0.5f;
    
    bool  Pair0         = (Gradient0 >= Gradient1);
    float LocalLumaAvg  = (!Pair0) ? LumaAvg1 : LumaAvg0;
    float LocalGradient = (!Pair0) ? Gradient1 : Gradient0;
    LocalGradient = LocalGradient * FXAA_SEARCH_THRESHOLD;

    float StepLength = IsHorizontal ? -InvTextureSize.y : -InvTextureSize.x;
    if (!Pair0)
    {
        StepLength *= -1.0f;
    }
    
    float2 CurrentTexCoord = TexCoord;
    if (IsHorizontal)
    {
        CurrentTexCoord.y += StepLength * 0.5f;
    }
    else
    {
        CurrentTexCoord.x += StepLength * 0.5f;
    }
    
    float2 Offset    = IsHorizontal ? float2(InvTextureSize.x, 0.0f) : float2(0.0f, InvTextureSize.y);
    bool   Done0     = false;
    bool   Done1     = false;
    float  LumaEnd0  = LocalLumaAvg;
    float  LumaEnd1  = LocalLumaAvg;
    float2 TexCoord0 = CurrentTexCoord - Offset;
    float2 TexCoord1 = CurrentTexCoord + Offset;
    
    for (int Steps = 0; Steps < FXAA_SEARCH_STEPS; Steps++)
    {
        if (!Done0)
        {
            float4 Sample = FXAASample(FinalImage, Sampler, TexCoord0);
            LumaEnd0 = Sample.a;
        }
        if(!Done1)
        {
            float4 Sample = FXAASample(FinalImage, Sampler, TexCoord1);
            LumaEnd1 = Sample.a;
        }
        
        Done0 = (abs(LumaEnd0 - LocalLumaAvg) >= LocalGradient);
        Done1 = (abs(LumaEnd1 - LocalLumaAvg) >= LocalGradient);
        if (Done0 && Done1)
        {
            break;
        }
        
        if (!Done0)
        {
            TexCoord0 -= Offset;
        }
        if (!Done1)
        {
            TexCoord1 += Offset;
        }
    }
    
    float Distance0 = IsHorizontal ? (TexCoord.x - TexCoord0.x) : (TexCoord.y - TexCoord0.y);
    float Distance1 = IsHorizontal ? (TexCoord1.x - TexCoord.x) : (TexCoord1.y - TexCoord.y);
    
    bool Dir0 = Distance0 < Distance1;
#if DEBUG_NEGPOS
    if(Dir0)
    {
        return float4(1.0f, 0.0f, 0.0f, 1.0f);
    }
    else
    {
        return float4(0.0f, 0.0f, 1.0f, 1.0f);
    }
#endif

    LumaEnd0 = Dir0 ? LumaEnd0 : LumaEnd1;
    if (((LumaM - LocalLumaAvg) < 0.0f) == ((LumaEnd0 - LocalLumaAvg) < 0.0f))
    {
        StepLength = 0.0f;
    }
    
#if DEBUG_STEP
    return ToFloat4(StepLength);
#endif
    
    float SpanLength = (Distance0 + Distance1);
    Distance0        = Dir0 ? Distance0 : Distance1;
    
#if DEBUG_STEP
    return ToFloat4(StepLength);
#endif
    
    float  SubPixelOffset = (0.5f + (Distance0 * (-1.0f / SpanLength))) * StepLength;
    float2 FinalTexCoord  = TexCoord + float2(IsHorizontal ? 0.0f : SubPixelOffset, IsHorizontal ? SubPixelOffset : 0.0f);
    float3 ColorF         = FXAASample(FinalImage, Sampler, FinalTexCoord).rgb;
    float3 FinalColor     = Lerp(ColorL, ColorF, BlendL);
    return float4(FinalColor, 1.0f);
}