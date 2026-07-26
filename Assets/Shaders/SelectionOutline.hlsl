#include "CoreDefines.hlsli"

Texture2D<uint>        ObjectID         : register(t0);
StructuredBuffer<uint> SelectedIDs      : register(t1);
Texture2D<float>       InputMask        : register(t2);
Texture2D<float>       DilatedMaskInput : register(t3);

SamplerState LinearSampler : register(s0);

SHADER_CONSTANT_BLOCK_BEGIN
    // 0-16
    int2 ScreenSize;
    int2 Direction;

    // 16-32
    int    Radius;
    uint   SelectedCount;
    float2 InvViewport;

    // 32-40
    float Smoothness;
    float Padding0;
SHADER_CONSTANT_BLOCK_END

// ------------------------------------------------------------------------------------------------
// Selection Mask
// ------------------------------------------------------------------------------------------------

float SelectionMaskPS(float2 /* TexCoord */ : TEXCOORD0, float4 Position : SV_Position) : SV_Target0
{
    const uint2 Pixel = uint2(Position.xy);

    const uint ObjectIDValue = ObjectID.Load(int3(Pixel, 0)).r;
    if (ObjectIDValue == 0 || Constants.SelectedCount == 0)
    {
        return 0.0f;
    }

    float Selected = 0.0f;
    const uint SafeSelectedCount = min(Constants.SelectedCount, 64u);

    [loop]
    for (uint i = 0; i < SafeSelectedCount; ++i)
    {
        if (SelectedIDs[i] == ObjectIDValue)
        {
            Selected = 1.0f;
            break;
        }
    }

    return Selected;
}

// ------------------------------------------------------------------------------------------------
// Dilate (Max)
// ------------------------------------------------------------------------------------------------

float DilateMaxPS(float2 /* TexCoord */ : TEXCOORD0, float4 Position : SV_Position) : SV_Target0
{
    const int2 Pixel = int2(Position.xy);

    const int2 MinPixel = int2(0, 0);
    const int2 MaxPixel = Constants.ScreenSize - 1;

    float Result = 0.0f;
    [loop]
    for (int Offset = -Constants.Radius; Offset <= Constants.Radius; ++Offset)
    {
        int2 SamplePixel = Pixel + (Constants.Direction * Offset);
        SamplePixel = clamp(SamplePixel, MinPixel, MaxPixel);

        Result = max(Result, InputMask.Load(int3(SamplePixel, 0)).r);
    }

    return Result;
}

// ------------------------------------------------------------------------------------------------
// Erode (Min)
// ------------------------------------------------------------------------------------------------

float ErodeMinPS(float2 /* TexCoord */ : TEXCOORD0, float4 Position : SV_Position) : SV_Target0
{
    const int2 Pixel = int2(Position.xy);

    const int2 MinPixel = int2(0, 0);
    const int2 MaxPixel = Constants.ScreenSize - 1;

    float Result = 1.0f;
    [loop]
    for (int Offset = -Constants.Radius; Offset <= Constants.Radius; ++Offset)
    {
        int2 SamplePixel = Pixel + (Constants.Direction * Offset);
        SamplePixel = clamp(SamplePixel, MinPixel, MaxPixel);

        Result = min(Result, InputMask.Load(int3(SamplePixel, 0)).r);
    }

    return Result;
}

// ------------------------------------------------------------------------------------------------
// Selection Ring
// ------------------------------------------------------------------------------------------------

float SelectionRingPS(float2 /* TexCoord */ : TEXCOORD0, float4 Position : SV_Position) : SV_Target0
{
    const float2 UV = (Position.xy + 0.5f) * Constants.InvViewport;

    const uint2 Pixel = uint2(Position.xy);
    const float InnerPoint = InputMask.Load(int3(Pixel, 0)).r;
    const float OuterPoint = DilatedMaskInput.Load(int3(Pixel, 0)).r;
    const float RingPoint  = saturate(OuterPoint - InnerPoint);

    // Ring = Outer - Inner (anti-aliased using SSAA).
    // Smoothness controls both the tap radius and how much we blend towards SSAA.
    if (Constants.Smoothness > 0.0f)
    {
        const float Smoothness = max(Constants.Smoothness, 0.0f);

        // Keep default (1.0) fairly crisp; reach full SSAA blend around 2.0.
        const float Blend = saturate(Smoothness * 0.5f);

        // Subpixel tap pattern (in pixels). Use a smaller radius for the low-smoothness case.
        const float2 TapOffset = Constants.InvViewport * (0.45f * Smoothness);

        float InnerSSAA = 0.0f;
        float OuterSSAA = 0.0f;

        if (Smoothness < 1.5f)
        {
            const float2 Offsets[5] =
            {
                float2(0.0f, 0.0f),
                float2(-TapOffset.x, 0.0f),
                float2( TapOffset.x, 0.0f),
                float2(0.0f, -TapOffset.y),
                float2(0.0f,  TapOffset.y),
            };

            [unroll]
            for (int i = 0; i < 5; ++i)
            {
                InnerSSAA += InputMask.SampleLevel(LinearSampler, UV + Offsets[i], 0).r;
                OuterSSAA += DilatedMaskInput.SampleLevel(LinearSampler, UV + Offsets[i], 0).r;
            }

            InnerSSAA *= (1.0f / 5.0f);
            OuterSSAA *= (1.0f / 5.0f);
        }
        else
        {
            const float2 Offsets[9] =
            {
                float2(0.0f, 0.0f),

                float2(-TapOffset.x, 0.0f),
                float2( TapOffset.x, 0.0f),
                float2(0.0f, -TapOffset.y),
                float2(0.0f,  TapOffset.y),

                float2(-TapOffset.x, -TapOffset.y),
                float2( TapOffset.x, -TapOffset.y),
                float2(-TapOffset.x,  TapOffset.y),
                float2( TapOffset.x,  TapOffset.y),
            };

            [unroll]
            for (int i = 0; i < 9; ++i)
            {
                InnerSSAA += InputMask.SampleLevel(LinearSampler, UV + Offsets[i], 0).r;
                OuterSSAA += DilatedMaskInput.SampleLevel(LinearSampler, UV + Offsets[i], 0).r;
            }

            InnerSSAA *= (1.0f / 9.0f);
            OuterSSAA *= (1.0f / 9.0f);
        }

        const float RingSSAA = saturate(OuterSSAA - InnerSSAA);
        return lerp(RingPoint, RingSSAA, Blend);
    }

    return RingPoint;
}
