#pragma once
#include "Core/Math/Vector3.h"
#include "Core/Math/Color.h"
#include "Core/Containers/StaticArray.h"
#include "Engine/EngineModule.h"
#include "RHI/RHIResources.h"

#define SafeGetDefaultSRV(Texture) (Texture ? Texture->GetShaderResourceView() : nullptr)

enum class EMaterialFlags : int32
{
    None                    = 0,       // No flags
    EnableHeight            = FLAG(0), // Enable HeightMaps (Parallax Occlusion Mapping)
    EnableAlpha             = FLAG(1), // Enable Alpha Textures (alpha in AlbedoMap.a)
    EnableNormalMapping     = FLAG(2), // Enable Normal Mapping
    EnableParallaxClipping  = FLAG(3), // Discard where the parallax offset leaves the UV tile.
    NormalMapPositiveY      = FLAG(4), // The NormalMap is authored green-up (+Y, "OpenGL") rather than green-down (-Y, "DirectX")
    DoubleSided             = FLAG(5), // The Material should be rendered without culling
    ForceForwardPass        = FLAG(6), // This material should be rendered in the ForwardPass
};

ENUM_CLASS_OPERATORS(EMaterialFlags);

struct FMaterialInfo
{
    FMaterialInfo()
        : Albedo(FFloatColor::White)
        , Roughness(0.0f)
        , Metallic(0.0f)
        , AmbientOcclusion(0.5f)
        , ParallaxHeightScale(0.03f)
        , ParallaxMinLayers(32.0f)
        , ParallaxMaxLayers(64.0f)
        , MaterialFlags(EMaterialFlags::None)

    {
    }

    FFloatColor    Albedo;
    float          Roughness;
    float          Metallic;
    float          AmbientOcclusion;
    float          ParallaxHeightScale;
    float          ParallaxMinLayers;
    float          ParallaxMaxLayers;
    EMaterialFlags MaterialFlags;
};

// Mirrored by the NORMAL_MAP_FLAG_* defines in Assets/Shaders/Structs.hlsli
enum class ENormalMapFlags : uint32
{
    None      = 0,
    Enabled   = (1u << 0), // A real normal map is bound, rather than the fallback flat one
    PositiveY = (1u << 1), // Green has to be flipped on the way in to reach the engine's basis
};

ENUM_CLASS_OPERATORS(ENormalMapFlags);

struct FMaterialHLSL
{
    // 0-16 
    Vector3 Albedo    = Vector3(1.0f);
    float   Roughness = 1.0f;
    
    // 16-32
    float                Metallic         = 0.0f;
    float                AmbientOcclusion = 1.0f;
    FRHIDescriptorHandle AlbedoHandle     = {};
    FRHIDescriptorHandle NormalHandle     = {};
    
    // 32-48
    float                ParallaxHeightScale = 0.03f;
    float                ParallaxMinLayers   = 32.0f;
    float                ParallaxMaxLayers   = 64.0f;
    FRHIDescriptorHandle MaterialHandle      = {};

    // 48-64
    FRHIDescriptorHandle HeightHandle   = {};
    FRHIDescriptorHandle SamplerHandle  = {};
    ENormalMapFlags      NormalMapFlags = ENormalMapFlags::None;
    uint32               Padding0       = 0;
};

static_assert(sizeof(FRHIDescriptorHandle) == sizeof(uint32), "FRHIDescriptorHandle must be 4 bytes for the HLSL Material layout");
static_assert(sizeof(FMaterialHLSL) == 64, "FMaterialHLSL must match the HLSL Material layout");

class ENGINE_API FMaterial
{
public:
    FMaterial(const FMaterialInfo& InMaterialInfo);
    ~FMaterial();

    void Initialize();
    void FillMaterialData(FMaterialHLSL& OutData) const;

    void SetAlbedo(const FFloatColor& Albedo);
    void SetMetallic(float Metallic);
    void SetRoughness(float Roughness);
    void SetAmbientOcclusion(float AO);
    void SetMaterialFlags(EMaterialFlags InFlags, bool bUpdateOnly = false);
    
    void ForceForwardPass(bool bForceForwardRender);
    
    void EnableHeightMap(bool bEnableHeightMap);
    void EnableAlphaMask(bool bEnableAlphaMask);
    void EnableNormalMapping(bool bEnableNormalMapping);
    void SetNormalMapPositiveY(bool bPositiveY);
    void EnableDoubleSided(bool bIsDoubleSided);
    void EnableParallaxClipping(bool bEnableParallaxClipping);

    void SetParallaxHeightScale(float InParallaxHeightScale);
    void SetParallaxLayers(float InParallaxMinLayers, float InParallaxMaxLayers);
    
    void SetName(const String& InName);

    bool HasAlphaMask()         const { return IsEnumFlagSet(MaterialInfo.MaterialFlags, EMaterialFlags::EnableAlpha); }
    bool HasHeightMap()         const { return IsEnumFlagSet(MaterialInfo.MaterialFlags, EMaterialFlags::EnableHeight) && HeightMap.IsValid(); }
    bool HasNormalMap()         const { return IsEnumFlagSet(MaterialInfo.MaterialFlags, EMaterialFlags::EnableNormalMapping); }
    bool IsNormalMapPositiveY() const { return IsEnumFlagSet(MaterialInfo.MaterialFlags, EMaterialFlags::NormalMapPositiveY); }
    bool HasParallaxClipping()  const { return IsEnumFlagSet(MaterialInfo.MaterialFlags, EMaterialFlags::EnableParallaxClipping) && HasHeightMap(); }

    bool IsDoubleSided() const { return IsEnumFlagSet(MaterialInfo.MaterialFlags, EMaterialFlags::DoubleSided); }

    bool ShouldRenderInForwardPass() const { return IsEnumFlagSet(MaterialInfo.MaterialFlags, EMaterialFlags::ForceForwardPass); }
    bool ShouldRenderInPrePass()     const { return !ShouldRenderInForwardPass(); }

    bool SupportsPixelDiscard() const { return HasHeightMap() || HasAlphaMask(); }

    FRHISamplerState* GetMaterialSampler() const
    {
        return Sampler.Get();
    }

    // Index into the global shared material StructuredBuffer.
    int32 GetBufferIndex() const
    {
        return BufferIndex;
    }

    void SetBufferIndex(int32 InBufferIndex)
    {
        BufferIndex = InBufferIndex;
    }

    EMaterialFlags GetMaterialFlags() const 
    {
        EMaterialFlags Flags = MaterialInfo.MaterialFlags;
        if (!HeightMap.IsValid())
        {
            Flags &= ~(EMaterialFlags::EnableHeight | EMaterialFlags::EnableParallaxClipping);
        }

        return Flags;
    }

    const FMaterialInfo& GetMaterialInfo() const
    {
        return MaterialInfo;
    }
    
    const String& GetName() const
    {
        return Name;
    }

    float GetParallaxHeightScale() const
    {
        return MaterialInfo.ParallaxHeightScale;
    }

    float GetParallaxMinLayers() const
    {
        return MaterialInfo.ParallaxMinLayers;
    }

    float GetParallaxMaxLayers() const
    {
        return MaterialInfo.ParallaxMaxLayers;
    }

public:
    FRHITextureRef AlbedoMap;   // RGB=BaseColor, A=Opacity
    FRHITextureRef NormalMap;   // Tangent-space normal (BC5)
    FRHITextureRef MaterialMap; // R=AO, G=Roughness, B=Metallic (BC1)
    FRHITextureRef HeightMap;   // Parallax height (BC4)

private:
    String              Name;
    FMaterialInfo       MaterialInfo;
    int32               BufferIndex = 0;
    FRHISamplerStateRef Sampler;
};
