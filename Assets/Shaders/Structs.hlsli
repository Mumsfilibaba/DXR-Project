#ifndef STRUCTS_HLSLI
#define STRUCTS_HLSLI

#include "CoreDefines.hlsli"

struct FCamera
{
    // 0-64
    float4x4 PrevViewProjection;
    // 64-128
    float4x4 ViewProjection;
    // 128-192
    float4x4 ViewProjectionInv;
    // 192-256
    float4x4 ViewProjectionUnjittered;
    // 256-320
    float4x4 ViewProjectionInvUnjittered;
    // 320-384
    float4x4 View;
    // 384-448
    float4x4 ViewInv;
    // 448-512
    float4x4 Projection;
    // 512-576
    float4x4 ProjectionInv;
    // 576-640
    float4x4 ProjectionUnjittered;
    // 640-704
    float4x4 ProjectionInvUnjittered;
    // 704-720
    float3   PositionWS;
    float    NearPlane;
    // 720-736
    float3   Forward;
    float    FarPlane;
    // 736-752
    float3   Right;
    float    AspectRatio;
    // 752-768
    // Offset baked into ViewProjection/PrevViewProjection. Zero when the rasterizer applies the jitter.
    float2   ProjectionJitter;
    float2   PrevProjectionJitter;
    // 768-784
    float    ViewportWidth;
    float    ViewportHeight;
    // Offset present in the rendered image, however it was produced.
    float2   ImageJitter;
    // 784-800
    float3   PrevPositionWS;
    float    Padding2;
};

struct FPositionRadius
{
    // 0-16
    float3 Position;
    float  Radius;
};

struct FPointLight
{
    // 0-16
    float3 Color;
    float  Padding0;
};

struct FShadowPointLight
{
    // 0-16
    float3 Color;
    float  ShadowBias;
    // 16-32
    float FarPlane;
    float Padding0;
    float Padding1;
    float Padding2;
};

// Mirrors FVertexAttributes in Runtime/Engine/Assets/VertexFormat.h.
struct FVertexAttributes
{
    // 0-8
    uint2 PackedNormal;  // R16G16B16A16_Snorm, w unused
    // 8-16
    uint2 PackedTangent; // R16G16B16A16_Snorm, w carries the tangent handedness sign
    // 16-24
    float2 TexCoord;
};

struct FPerObject
{
    // 0-48
    row_major float3x4 LocalToWorld;
    // 48-96
    row_major float3x4 TransformInvT;
    // 96-112
    uint  ObjectID;
    uint  MaterialIndex;
    float DeterminantSign;
    uint  Padding2;
};

// Mirrors ENormalMapFlags in Runtime/Engine/Resources/Material.h
#define NORMAL_MAP_FLAG_ENABLED     (1)
#define NORMAL_MAP_FLAG_POSITIVE_Y  (2)
#define NORMAL_MAP_FLAG_TWO_CHANNEL (4)

// Mirrors EMaterialTextureSlot in Runtime/Engine/Resources/Material.h
#define MATERIAL_SLOT_BASE_COLOR (0)
#define MATERIAL_SLOT_NORMAL     (1)
#define MATERIAL_SLOT_HEIGHT     (2)
#define MATERIAL_SLOT_MASK_A     (3)
#define MATERIAL_SLOT_MASK_B     (4)
#define MATERIAL_SLOT_MASK_C     (5)
#define MATERIAL_SLOT_MASK_D     (6)
#define MATERIAL_SLOT_COUNT      (7)

// Mirrors EMaterialScalar. One route byte each, in this order, inside FMaterial::ScalarRoutes.
#define MATERIAL_SCALAR_ROUGHNESS (0)
#define MATERIAL_SCALAR_METALLIC  (1)
#define MATERIAL_SCALAR_OCCLUSION (2)
#define MATERIAL_SCALAR_OPACITY   (3)

// A route byte is slot in bits 0-3, channel in bits 4-5 and invert in bit 6.
#define MATERIAL_ROUTE_SLOT_NONE (0xFu)

struct FMaterial
{
    // 0-16
    float3 Albedo;
    float  Roughness;
    // 16-32
    float Metallic;
    float AO;
    float ParallaxHeightScale;
    float ParallaxMinLayers;
    // 32-48
    float ParallaxMaxLayers;
    uint  ScalarRoutes;
    uint  NormalMapFlags;
    float Opacity;
    // 48-80
    uint  SlotHandles[MATERIAL_SLOT_COUNT];
    uint  SamplerHandle;
    // 80-96
    float IndexOfRefraction;
    float RefractionStrength;
    uint  Padding0;
    uint  Padding1;
};

bool HasNormalMap(FMaterial MaterialData)
{
    return (MaterialData.NormalMapFlags & NORMAL_MAP_FLAG_ENABLED) != 0;
}

bool IsNormalMapPositiveY(FMaterial MaterialData)
{
    return (MaterialData.NormalMapFlags & NORMAL_MAP_FLAG_POSITIVE_Y) != 0;
}

bool IsNormalMapTwoChannel(FMaterial MaterialData)
{
    return (MaterialData.NormalMapFlags & NORMAL_MAP_FLAG_TWO_CHANNEL) != 0;
}

uint GetMaterialScalarRoute(FMaterial MaterialData, uint Scalar)
{
    return (MaterialData.ScalarRoutes >> (Scalar * 8)) & 0xFFu;
}

uint GetMaterialRouteSlot(uint Route)
{
    return Route & 0xFu;
}

uint GetMaterialRouteChannel(uint Route)
{
    return (Route >> 4) & 0x3u;
}

bool IsMaterialRouteInverted(uint Route)
{
    return ((Route >> 6) & 0x1u) != 0;
}

struct FLightProbeInfo
{
    // 0-16
    float3 BoxOriginWS;
    float  BoxProjection;
    // 16-32
    float3 BoxMinWS;
    float  Padding0;
    // 32-48
    float3 BoxMaxWS;
    float  Padding1;
};

#endif
