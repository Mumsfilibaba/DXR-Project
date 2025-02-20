#include "Structs.hlsli"
#include "Constants.hlsli"

// Based on GenerateMipsCS.hlsli by Microsoft
// https://github.com/Microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/GenerateMipsCS.hlsli

//#define CONFIG_CUBE_MAP			1
#define BLOCK_SIZE (8)
#define CHANNEL_COMPONENTS (BLOCK_SIZE * BLOCK_SIZE)
#define CONFIG_POWER_OF_TWO (1)

// Input
#if CONFIG_CUBE_MAP
    TextureCube<float4> SourceMip : register(t0);
#else
    Texture2D<float4> SourceMip : register(t0);
#endif

// Output
#if CONFIG_CUBE_MAP
    RWTexture2DArray<float4> OutputMip1 : register(u0);
    RWTexture2DArray<float4> OutputMip2 : register(u1);
    RWTexture2DArray<float4> OutputMip3 : register(u2);
    RWTexture2DArray<float4> OutputMip4 : register(u3);
#else
    RWTexture2D<float4> OutputMip1 : register(u0);
    RWTexture2D<float4> OutputMip2 : register(u1);
    RWTexture2D<float4> OutputMip3 : register(u2);
    RWTexture2D<float4> OutputMip4 : register(u3);
#endif

// Linear sampler
SamplerState LinearSampler : register(s0);

// Properties
SHADER_CONSTANT_BLOCK_BEGIN
    uint   SrcMipLevel;   // Texture level of source mip
    uint   NumMipLevels;  // Number of OutMips to write: [1, 4]
    float2 TexelSize;     // 1.0 / OutputMip1.Dimensions
SHADER_CONSTANT_BLOCK_END

// The reason for separating channels is to reduce bank conflicts in the
// local data memory controller. A large stride will cause more threads
// to collide on the same memory bank.
groupshared float RedChannel[CHANNEL_COMPONENTS];
groupshared float GreenChannel[CHANNEL_COMPONENTS];
groupshared float BlueChannel[CHANNEL_COMPONENTS];
groupshared float AlphaChannel[CHANNEL_COMPONENTS];

void StoreColor(uint Index, float4 Color)
{
    RedChannel[Index]	= Color.r;
    GreenChannel[Index] = Color.g;
    BlueChannel[Index]	= Color.b;
    AlphaChannel[Index] = Color.a;
}

float4 LoadColor(uint Index)
{
    return float4(RedChannel[Index], GreenChannel[Index], BlueChannel[Index], AlphaChannel[Index]);
}

float3 ApplySRGBCurve(float3 x)
{
    // This is exactly the sRGB curve
    //return x < 0.0031308 ? 12.92 * x : 1.055 * pow(abs(x), 1.0 / 2.4) - 0.055;

    // This is cheaper but nearly equivalent
    const bool3 bResult = (x < 0.0031308f);
    return any(bResult) ? (12.92f * x) : (1.13005f * sqrt(abs(x - 0.00228f)) - 0.13448f * x + 0.005719f);
}

float4 PackColor(float4 Linear)
{
#ifdef CONVERT_TO_SRGB
    return float4(ApplySRGBCurve(Linear.rgb), Linear.a);
#else
    return Linear;
#endif
}

#if CONFIG_CUBE_MAP
// Transform from dispatch ID to cubemap face direction
static const float3x3 RotateUV[6] =
{
    // +X
    float3x3( 0,  0, 1,
              0, -1, 0,
             -1,  0, 0),
    // -X
    float3x3( 0,  0, -1,
              0, -1,  0,
              1,  0,  0),
    // +Y
    float3x3( 1, 0, 0,
              0, 0, 1,
              0, 1, 0),
    // -Y
    float3x3( 1,  0,  0,
              0,  0, -1,
              0, -1,  0),
    // +Z
    float3x3( 1,  0, 0,
              0, -1, 0,
              0,  0, 1),
    // -Z
    float3x3(-1,  0,  0,
              0, -1,  0,
              0,  0, -1)
};
#endif

[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void Main(FComputeShaderInput Input)
{
    // One bilinear sample is insufficient when scaling down by more than 2x.
    // You will slightly undersample in the case where the source dimension
    // is odd.  This is why it's a really good idea to only generate mips on
    // power-of-two sized textures.  Trying to handle the undersampling case
    // will force this shader to be slower and more complicated as it will
    // have to take more source texture samples.
#if CONFIG_CUBE_MAP
    float3 TexCoord = float3((Input.DispatchThreadID.xy * Constants.TexelSize) - 0.5f, 0.5f);
    TexCoord = normalize(mul(RotateUV[Input.DispatchThreadID.z], TexCoord));
    float4 Src1 = SourceMip.SampleLevel(LinearSampler, TexCoord, Constants.SrcMipLevel);
#elif CONFIG_POWER_OF_TWO
    float2 TexCoord = Constants.TexelSize * (Input.DispatchThreadID.xy + 0.5f);
    float4 Src1     = SourceMip.SampleLevel(LinearSampler, TexCoord, Constants.SrcMipLevel);
#else
    #error "Not supported yet"
#endif

#if CONFIG_CUBE_MAP
    OutputMip1[Input.DispatchThreadID] = PackColor(Src1);
#else
    OutputMip1[Input.DispatchThreadID.xy] = PackColor(Src1);
#endif

    // A scalar (constant) branch can exit all threads coherently.
    if (Constants.NumMipLevels == 1)
    {
        return;
    }

    // Without lane swizzle operations, the only way to share data with other
    // threads is through LDS.
    StoreColor(Input.GroupIndex, Src1);

    // This guarantees all LDS writes are complete and that all threads have
    // executed all instructions so far (and therefore have issued their LDS
    // write instructions.)
    GroupMemoryBarrierWithGroupSync();

    // With low three bits for X and high three bits for Y, this bit mask
    // (binary: 001001) checks that X and Y are even.
    if ((Input.GroupIndex & 0x9) == 0)
    {
        float4 Src2 = LoadColor(Input.GroupIndex + 0x01);
        float4 Src3 = LoadColor(Input.GroupIndex + 0x08);
        float4 Src4 = LoadColor(Input.GroupIndex + 0x09);
        Src1 = 0.25f * (Src1 + Src2 + Src3 + Src4);

    #if CONFIG_CUBE_MAP
        OutputMip2[uint3(Input.DispatchThreadID.xy / 2, Input.DispatchThreadID.z)] = PackColor(Src1);
    #else
        OutputMip2[Input.DispatchThreadID.xy / 2] = PackColor(Src1);
    #endif
        StoreColor(Input.GroupIndex, Src1);
    }

    if (Constants.NumMipLevels == 2)
    {
        return;
    }

    GroupMemoryBarrierWithGroupSync();

    // This bit mask (binary: 011011) checks that X and Y are multiples of four.
    if ((Input.GroupIndex & 0x1B) == 0)
    {
        float4 Src2 = LoadColor(Input.GroupIndex + 0x02);
        float4 Src3 = LoadColor(Input.GroupIndex + 0x10);
        float4 Src4 = LoadColor(Input.GroupIndex + 0x12);
        Src1 = 0.25f * (Src1 + Src2 + Src3 + Src4);

    #if CONFIG_CUBE_MAP
        OutputMip3[uint3(Input.DispatchThreadID.xy / 4, Input.DispatchThreadID.z)] = PackColor(Src1);
    #else
        OutputMip3[Input.DispatchThreadID.xy / 4] = PackColor(Src1);
    #endif
        StoreColor(Input.GroupIndex, Src1);
    }

    if (Constants.NumMipLevels == 3)
    {
        return;
    }

    GroupMemoryBarrierWithGroupSync();

    // This bit mask would be 111111 (X & Y multiples of 8), but only one
    // thread fits that criteria.
    if (Input.GroupIndex == 0)
    {
        float4 Src2 = LoadColor(Input.GroupIndex + 0x04);
        float4 Src3 = LoadColor(Input.GroupIndex + 0x20);
        float4 Src4 = LoadColor(Input.GroupIndex + 0x24);
        Src1 = 0.25f * (Src1 + Src2 + Src3 + Src4);

    #if CONFIG_CUBE_MAP
        OutputMip4[uint3(Input.DispatchThreadID.xy / 8, Input.DispatchThreadID.z)] = PackColor(Src1);
    #else
        OutputMip4[Input.DispatchThreadID.xy / 8] = PackColor(Src1);
    #endif
    }
}
