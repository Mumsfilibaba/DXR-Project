#ifndef MATERIAL_SAMPLING_HLSLI
#define MATERIAL_SAMPLING_HLSLI

// ------------------------------------------------------------------------------------------------
//  The one place that knows how a material's textures are reached and decoded.
//  A pass says where its material resources live and includes this. It never writes the
//  bindless-versus-bound branch itself:
//      #define MATERIAL_SRV_REGISTER_BASE 0
//      #define MATERIAL_ARRAY_REGISTER    t7
//      #include "MaterialSampling.hlsli"
//
//  The base is the number of the first of MATERIAL_SLOT_COUNT consecutive t registers, not the
//  register token, because this header names all of them from it.
//  ENABLE_BINDLESS is already global, defaulted in CoreDefines.hlsli and driven by the
//  FBindless permutation dimension, so it needs no mention here.
//  Which slot and channel feeds each scalar property is runtime data, read from the material
//  buffer, so a pass gets the same answer whatever layout the source assets happened to use.
//
//  Optional knobs:
//    MATERIAL_SLOT_ACCESS        How the pass reaches its slots. Defaults to the heap when
//                                ENABLE_BINDLESS and to declared textures otherwise. A pass that
//                                resolves its own textures sets MATERIAL_SLOT_ACCESS_NONE and gets
//                                the decoding below and nothing else.
//    MATERIAL_SAMPLER_REGISTER   Sampler register for the bound path. Defaults to s0.
//    MATERIAL_SRV_REGISTER_SPACE Register space for the slots and their sampler, which the ray
//                                tracing hit shaders need because theirs are local root arguments.
//    MATERIAL_SAMPLE_NORMAL      Set to 0 when the pass has no normal mapping, so the sample is
//                                never emitted rather than left for the optimiser to remove.
//    MATERIAL_SLOT_NON_UNIFORM   Set to 1 when neighbouring lanes can be looking at different
//                                materials, which is the case wherever a ray decides the material.
//    MATERIAL_DECLARE_ARRAY      Set to 0 when the pass declares the material buffer itself.
// ------------------------------------------------------------------------------------------------

#include "CoreDefines.hlsli"
#include "Structs.hlsli"
#include "Helpers.hlsli"
#include "ColorSpaceTransforms.hlsli"
#include "ParallaxMapping.hlsli"
#include "TangentSpace.hlsli"

#define MATERIAL_SLOT_ACCESS_NONE     (0) // The pass brings its own textures
#define MATERIAL_SLOT_ACCESS_BOUND    (1) // Textures declared from MATERIAL_SRV_REGISTER_BASE
#define MATERIAL_SLOT_ACCESS_BINDLESS (2) // The descriptor heap, indexed by the slot handles

#ifndef MATERIAL_SLOT_ACCESS
    #if ENABLE_BINDLESS
        #define MATERIAL_SLOT_ACCESS MATERIAL_SLOT_ACCESS_BINDLESS
    #else
        #define MATERIAL_SLOT_ACCESS MATERIAL_SLOT_ACCESS_BOUND
    #endif
#endif

#if MATERIAL_SLOT_ACCESS == MATERIAL_SLOT_ACCESS_BINDLESS
    #include "BindlessHelpers.hlsli"
#endif

#ifndef MATERIAL_SAMPLE_NORMAL
    #define MATERIAL_SAMPLE_NORMAL (1)
#endif

#ifndef MATERIAL_SLOT_NON_UNIFORM
    #define MATERIAL_SLOT_NON_UNIFORM (0)
#endif

#ifndef MATERIAL_DECLARE_ARRAY
    #define MATERIAL_DECLARE_ARRAY (1)
#endif

#if MATERIAL_DECLARE_ARRAY
    #include "MaterialArray.hlsli"
#endif

#if MATERIAL_SLOT_ACCESS == MATERIAL_SLOT_ACCESS_BOUND
    #ifndef MATERIAL_SRV_REGISTER_BASE
        #error "MaterialSampling.hlsli needs MATERIAL_SRV_REGISTER_BASE to place the slot registers"
    #endif

    #ifndef MATERIAL_SAMPLER_REGISTER
        #define MATERIAL_SAMPLER_REGISTER s0
    #endif

    #if MATERIAL_SRV_REGISTER_BASE == 0
        #define MATERIAL_REGISTER_BASE_COLOR t0
        #define MATERIAL_REGISTER_NORMAL     t1
        #define MATERIAL_REGISTER_HEIGHT     t2
        #define MATERIAL_REGISTER_MASK_A     t3
        #define MATERIAL_REGISTER_MASK_B     t4
        #define MATERIAL_REGISTER_MASK_C     t5
        #define MATERIAL_REGISTER_MASK_D     t6
    #elif MATERIAL_SRV_REGISTER_BASE == 2
        #define MATERIAL_REGISTER_BASE_COLOR t2
        #define MATERIAL_REGISTER_NORMAL     t3
        #define MATERIAL_REGISTER_HEIGHT     t4
        #define MATERIAL_REGISTER_MASK_A     t5
        #define MATERIAL_REGISTER_MASK_B     t6
        #define MATERIAL_REGISTER_MASK_C     t7
        #define MATERIAL_REGISTER_MASK_D     t8
    #elif MATERIAL_SRV_REGISTER_BASE == 5
        #define MATERIAL_REGISTER_BASE_COLOR t5
        #define MATERIAL_REGISTER_NORMAL     t6
        #define MATERIAL_REGISTER_HEIGHT     t7
        #define MATERIAL_REGISTER_MASK_A     t8
        #define MATERIAL_REGISTER_MASK_B     t9
        #define MATERIAL_REGISTER_MASK_C     t10
        #define MATERIAL_REGISTER_MASK_D     t11
    #else
        #error "MaterialSampling.hlsli has no register run for this MATERIAL_SRV_REGISTER_BASE; add one above"
    #endif

    #ifdef MATERIAL_SRV_REGISTER_SPACE
        #define MATERIAL_REGISTER_CLAUSE(Register) register(Register, MATERIAL_SRV_REGISTER_SPACE)
    #else
        #define MATERIAL_REGISTER_CLAUSE(Register) register(Register)
    #endif

    Texture2D<float4> MaterialBaseColorTexture : MATERIAL_REGISTER_CLAUSE(MATERIAL_REGISTER_BASE_COLOR);
    Texture2D<float4> MaterialNormalTexture    : MATERIAL_REGISTER_CLAUSE(MATERIAL_REGISTER_NORMAL);
    Texture2D<float4> MaterialHeightTexture    : MATERIAL_REGISTER_CLAUSE(MATERIAL_REGISTER_HEIGHT);
    Texture2D<float4> MaterialMaskATexture     : MATERIAL_REGISTER_CLAUSE(MATERIAL_REGISTER_MASK_A);
    Texture2D<float4> MaterialMaskBTexture     : MATERIAL_REGISTER_CLAUSE(MATERIAL_REGISTER_MASK_B);
    Texture2D<float4> MaterialMaskCTexture     : MATERIAL_REGISTER_CLAUSE(MATERIAL_REGISTER_MASK_C);
    Texture2D<float4> MaterialMaskDTexture     : MATERIAL_REGISTER_CLAUSE(MATERIAL_REGISTER_MASK_D);
    SamplerState      MaterialSampler          : MATERIAL_REGISTER_CLAUSE(MATERIAL_SAMPLER_REGISTER);
#endif

#define MATERIAL_HAS_SLOT_ACCESS (MATERIAL_SLOT_ACCESS != MATERIAL_SLOT_ACCESS_NONE)

float4 GetMaterialSlotDefault(uint Slot)
{
    return (Slot == MATERIAL_SLOT_NORMAL) ? float4(0.5, 0.5, 1.0, 1.0) : float4(1.0, 1.0, 1.0, 1.0);
}

#if MATERIAL_SLOT_ACCESS == MATERIAL_SLOT_ACCESS_BINDLESS
    #if MATERIAL_SLOT_NON_UNIFORM
        #define DECLARE_MATERIAL_SLOT_TEXTURE(Name, Material, Slot) \
            Texture2D<float4> Name = GetResourceFromPackedDescriptorIndexNonUniform(Material.SlotHandles[min(uint(Slot), uint(MATERIAL_SLOT_COUNT - 1))])
    #else
        #define DECLARE_MATERIAL_SLOT_TEXTURE(Name, Material, Slot) \
            Texture2D<float4> Name = GetResourceFromPackedDescriptorIndex(Material.SlotHandles[min(uint(Slot), uint(MATERIAL_SLOT_COUNT - 1))])
    #endif
#endif

#if MATERIAL_HAS_SLOT_ACCESS

bool IsMaterialSlotValid(FMaterial Material, uint Slot)
{
    if (Slot >= uint(MATERIAL_SLOT_COUNT))
    {
        return false;
    }

#if MATERIAL_SLOT_ACCESS == MATERIAL_SLOT_ACCESS_BINDLESS
    return FDescriptorHandle::FromPacked(Material.SlotHandles[Slot]).IsValid();
#else
    return true;
#endif
}

SamplerState GetMaterialSampler(FMaterial Material)
{
#if MATERIAL_SLOT_ACCESS == MATERIAL_SLOT_ACCESS_BINDLESS
    return GetSamplerFromPackedDescriptorIndex(Material.SamplerHandle);
#else
    return MaterialSampler;
#endif
}

#if MATERIAL_SLOT_ACCESS == MATERIAL_SLOT_ACCESS_BOUND

float4 SampleBoundMaterialSlot(uint Slot, SamplerState Sampler, float2 UV)
{
    switch (Slot)
    {
        case MATERIAL_SLOT_BASE_COLOR: return MaterialBaseColorTexture.Sample(Sampler, UV);
        case MATERIAL_SLOT_NORMAL:     return MaterialNormalTexture.Sample(Sampler, UV);
        case MATERIAL_SLOT_HEIGHT:     return MaterialHeightTexture.Sample(Sampler, UV);
        case MATERIAL_SLOT_MASK_A:     return MaterialMaskATexture.Sample(Sampler, UV);
        case MATERIAL_SLOT_MASK_B:     return MaterialMaskBTexture.Sample(Sampler, UV);
        case MATERIAL_SLOT_MASK_C:     return MaterialMaskCTexture.Sample(Sampler, UV);
        default:                       return MaterialMaskDTexture.Sample(Sampler, UV);
    }
}

float4 SampleBoundMaterialSlotLevel(uint Slot, SamplerState Sampler, float2 UV, float Lod)
{
    switch (Slot)
    {
        case MATERIAL_SLOT_BASE_COLOR: return MaterialBaseColorTexture.SampleLevel(Sampler, UV, Lod);
        case MATERIAL_SLOT_NORMAL:     return MaterialNormalTexture.SampleLevel(Sampler, UV, Lod);
        case MATERIAL_SLOT_HEIGHT:     return MaterialHeightTexture.SampleLevel(Sampler, UV, Lod);
        case MATERIAL_SLOT_MASK_A:     return MaterialMaskATexture.SampleLevel(Sampler, UV, Lod);
        case MATERIAL_SLOT_MASK_B:     return MaterialMaskBTexture.SampleLevel(Sampler, UV, Lod);
        case MATERIAL_SLOT_MASK_C:     return MaterialMaskCTexture.SampleLevel(Sampler, UV, Lod);
        default:                       return MaterialMaskDTexture.SampleLevel(Sampler, UV, Lod);
    }
}

#endif // MATERIAL_SLOT_ACCESS == MATERIAL_SLOT_ACCESS_BOUND

float4 SampleMaterialSlot(FMaterial Material, uint Slot, float2 UV)
{
    if (!IsMaterialSlotValid(Material, Slot))
    {
        return GetMaterialSlotDefault(Slot);
    }

#if MATERIAL_SLOT_ACCESS == MATERIAL_SLOT_ACCESS_BOUND
    return SampleBoundMaterialSlot(Slot, GetMaterialSampler(Material), UV);
#else
    DECLARE_MATERIAL_SLOT_TEXTURE(SlotTexture, Material, Slot);
    return SlotTexture.Sample(GetMaterialSampler(Material), UV);
#endif
}

float4 SampleMaterialSlotLevel(FMaterial Material, uint Slot, float2 UV, float Lod)
{
    if (!IsMaterialSlotValid(Material, Slot))
    {
        return GetMaterialSlotDefault(Slot);
    }

#if MATERIAL_SLOT_ACCESS == MATERIAL_SLOT_ACCESS_BOUND
    return SampleBoundMaterialSlotLevel(Slot, GetMaterialSampler(Material), UV, Lod);
#else
    DECLARE_MATERIAL_SLOT_TEXTURE(SlotTexture, Material, Slot);
    return SlotTexture.SampleLevel(GetMaterialSampler(Material), UV, Lod);
#endif
}

#endif // MATERIAL_HAS_SLOT_ACCESS

// ------------------------------------------------------------------------------------------------
//  Routing
// ------------------------------------------------------------------------------------------------

float SelectRoutedChannel(uint Route, float4 Texel)
{
    const uint   Channel  = GetMaterialRouteChannel(Route);
    const float4 Selector = float4(Channel == 0, Channel == 1, Channel == 2, Channel == 3);
    const float  Value    = dot(Texel, Selector);

    return IsMaterialRouteInverted(Route) ? (1.0 - Value) : Value;
}

#if MATERIAL_HAS_SLOT_ACCESS

float SampleRoutedScalar(FMaterial Material, uint Scalar, float2 UV, float Factor)
{
    const uint Route = GetMaterialScalarRoute(Material, Scalar);
    const uint Slot  = GetMaterialRouteSlot(Route);

    if (Slot == MATERIAL_ROUTE_SLOT_NONE)
    {
        return Factor;
    }

    return SelectRoutedChannel(Route, SampleMaterialSlot(Material, Slot, UV)) * Factor;
}

float SampleRoutedScalarLevel(FMaterial Material, uint Scalar, float2 UV, float Lod, float Factor)
{
    const uint Route = GetMaterialScalarRoute(Material, Scalar);
    const uint Slot  = GetMaterialRouteSlot(Route);

    if (Slot == MATERIAL_ROUTE_SLOT_NONE)
    {
        return Factor;
    }

    return SelectRoutedChannel(Route, SampleMaterialSlotLevel(Material, Slot, UV, Lod)) * Factor;
}

#endif // MATERIAL_HAS_SLOT_ACCESS

// ------------------------------------------------------------------------------------------------
//  Decoding
// ------------------------------------------------------------------------------------------------

float3 DecodeMaterialNormalTS(FMaterial Material, float3 Texel)
{
    const float3 TangentNormal = IsNormalMapTwoChannel(Material) ? UnpackNormalBC5(Texel) : UnpackNormal(Texel);
    return ApplyNormalMapAxis(TangentNormal, IsNormalMapPositiveY(Material));
}

struct FMaterialSurface
{
    float3 BaseColor;   // linear
    float  Opacity;
    float3 NormalTS;    // decoded, encoding-aware
    float  Roughness;
    float  Metallic;
    float  Occlusion;
};

// ------------------------------------------------------------------------------------------------
//  Sampling a whole surface
// ------------------------------------------------------------------------------------------------

#if MATERIAL_HAS_SLOT_ACCESS

FMaterialSurface SampleMaterialSurface(FMaterial Material, float2 UV)
{
    FMaterialSurface Surface;
    Surface.BaseColor = SRGBToLinear(SampleMaterialSlot(Material, MATERIAL_SLOT_BASE_COLOR, UV).rgb) * Material.Albedo;

#if MATERIAL_SAMPLE_NORMAL
    Surface.NormalTS = DecodeMaterialNormalTS(Material, SampleMaterialSlot(Material, MATERIAL_SLOT_NORMAL, UV).rgb);
#else
    Surface.NormalTS = DecodeMaterialNormalTS(Material, GetMaterialSlotDefault(MATERIAL_SLOT_NORMAL).rgb);
#endif

    Surface.Opacity   = SampleRoutedScalar(Material, MATERIAL_SCALAR_OPACITY, UV, 1.0);
    Surface.Roughness = SampleRoutedScalar(Material, MATERIAL_SCALAR_ROUGHNESS, UV, Material.Roughness);
    Surface.Metallic  = SampleRoutedScalar(Material, MATERIAL_SCALAR_METALLIC, UV, Material.Metallic);
    Surface.Occlusion = SampleRoutedScalar(Material, MATERIAL_SCALAR_OCCLUSION, UV, Material.AO);
    return Surface;
}

FMaterialSurface SampleMaterialSurfaceLevel(FMaterial Material, float2 UV, float Lod)
{
    FMaterialSurface Surface;
    Surface.BaseColor = SRGBToLinear(SampleMaterialSlotLevel(Material, MATERIAL_SLOT_BASE_COLOR, UV, Lod).rgb) * Material.Albedo;

#if MATERIAL_SAMPLE_NORMAL
    Surface.NormalTS = DecodeMaterialNormalTS(Material, SampleMaterialSlotLevel(Material, MATERIAL_SLOT_NORMAL, UV, Lod).rgb);
#else
    Surface.NormalTS = DecodeMaterialNormalTS(Material, GetMaterialSlotDefault(MATERIAL_SLOT_NORMAL).rgb);
#endif

    Surface.Opacity   = SampleRoutedScalarLevel(Material, MATERIAL_SCALAR_OPACITY, UV, Lod, 1.0);
    Surface.Roughness = SampleRoutedScalarLevel(Material, MATERIAL_SCALAR_ROUGHNESS, UV, Lod, Material.Roughness);
    Surface.Metallic  = SampleRoutedScalarLevel(Material, MATERIAL_SCALAR_METALLIC, UV, Lod, Material.Metallic);
    Surface.Occlusion = SampleRoutedScalarLevel(Material, MATERIAL_SCALAR_OCCLUSION, UV, Lod, Material.AO);
    return Surface;
}

float SampleMaterialOpacity(FMaterial Material, float2 UV)
{
    return SampleRoutedScalar(Material, MATERIAL_SCALAR_OPACITY, UV, 1.0);
}

float2 ApplyMaterialParallax(FMaterial Material, float2 UV, float3 ViewDirTS, float2 Ddx, float2 Ddy, out bool bDiscard)
{
    bDiscard = false;

    if (!IsMaterialSlotValid(Material, MATERIAL_SLOT_HEIGHT))
    {
        return UV;
    }

    // Parallax marches the height field, so this one wants the texture itself rather than a sample.
#if MATERIAL_SLOT_ACCESS == MATERIAL_SLOT_ACCESS_BOUND
    return ParallaxMapUV(MaterialHeightTexture, GetMaterialSampler(Material), UV, ViewDirTS, Ddx, Ddy,
        Material.ParallaxHeightScale, Material.ParallaxMinLayers, Material.ParallaxMaxLayers, bDiscard);
#else
    DECLARE_MATERIAL_SLOT_TEXTURE(HeightTexture, Material, MATERIAL_SLOT_HEIGHT);
    return ParallaxMapUV(HeightTexture, GetMaterialSampler(Material), UV, ViewDirTS, Ddx, Ddy,
        Material.ParallaxHeightScale, Material.ParallaxMinLayers, Material.ParallaxMaxLayers, bDiscard);
#endif
}

#endif // MATERIAL_HAS_SLOT_ACCESS

#endif // MATERIAL_SAMPLING_HLSLI
