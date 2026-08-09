#pragma once
#include "Core/Math/Vector3.h"
#include "Core/Math/Color.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Misc/Asserts.h"
#include "Engine/EngineModule.h"
#include "RHI/RHIResources.h"

#define SafeGetDefaultSRV(Texture) (Texture ? Texture->GetShaderResourceView() : nullptr)

enum class EMaterialFlags : int32
{
    None                    = 0,       // No flags
    EnableHeight            = FLAG(0), // Enable HeightMaps (Parallax Occlusion Mapping)
    EnableAlpha             = FLAG(1), // Enable alpha masking, from wherever the opacity route points
    EnableNormalMapping     = FLAG(2), // Enable Normal Mapping
    EnableParallaxClipping  = FLAG(3), // Discard where the parallax offset leaves the UV tile.
    NormalMapPositiveY      = FLAG(4), // The NormalMap is authored green-up (+Y, "OpenGL") rather than green-down (-Y, "DirectX")
    DoubleSided             = FLAG(5), // The Material should be rendered without culling
    ForceForwardPass        = FLAG(6), // This material should be rendered in the ForwardPass
};

ENUM_CLASS_OPERATORS(EMaterialFlags);

class FVertexDeclaration;

// Mirrored by the MATERIAL_SLOT_* defines in Assets/Shaders/MaterialSampling.hlsli.
// Four mask slots is the worst case the importers can produce: occlusion, roughness, metallic and
// opacity arriving in separate files.
struct EMaterialTextureSlot
{
    enum Type : uint8
    {
        /** RGB base colour, alpha carries opacity when the opacity route points here */
        BaseColor = 0,

        /** Tangent-space normal */
        Normal,

        /** Parallax height */
        Height,

        MaskA,
        MaskB,
        MaskC,
        MaskD,
        Count,
    };
};

// Mirrored by the MATERIAL_SCALAR_* defines in Structs.hlsli.
struct EMaterialScalar
{
    enum Type : uint8
    {
        Roughness = 0,
        Metallic,
        Occlusion,
        Opacity,
        Count,
    };
};

enum class ETextureChannel : uint8
{
    R = 0,
    G,
    B,
    A,
};

struct FMaterialTextureRoute
{
    FMaterialTextureRoute() = default;

    FMaterialTextureRoute(EMaterialTextureSlot::Type InSlot, ETextureChannel InChannel, bool bInInvert = false)
        : Slot(InSlot)
        , Channel(InChannel)
        , bInvert(bInInvert)
    {
    }

    bool IsRouted() const
    {
        return Slot < EMaterialTextureSlot::Count;
    }

    EMaterialTextureSlot::Type Slot    = EMaterialTextureSlot::Count;
    ETextureChannel            Channel = ETextureChannel::R;
    bool                       bInvert = false; // Lets a gloss map feed roughness
};

// Four of these pack into FMaterialHLSL::ScalarRoutes, one byte per EMaterialScalar.
constexpr uint32 MATERIAL_ROUTE_SLOT_NONE = 0xFu;

static_assert(EMaterialTextureSlot::Count < MATERIAL_ROUTE_SLOT_NONE, "A slot index has to fit in the four bits a route byte gives it");

inline uint32 PackMaterialTextureRoute(const FMaterialTextureRoute& Route)
{
    if (!Route.IsRouted())
    {
        return MATERIAL_ROUTE_SLOT_NONE;
    }

    return uint32(Route.Slot) | (uint32(Route.Channel) << 4) | (Route.bInvert ? (1u << 6) : 0u);
}

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
        , Routes()
    {
        Routes[EMaterialScalar::Roughness] = FMaterialTextureRoute(EMaterialTextureSlot::MaskA, ETextureChannel::G);
        Routes[EMaterialScalar::Metallic]  = FMaterialTextureRoute(EMaterialTextureSlot::MaskA, ETextureChannel::B);
        Routes[EMaterialScalar::Occlusion] = FMaterialTextureRoute(EMaterialTextureSlot::MaskA, ETextureChannel::R);
        Routes[EMaterialScalar::Opacity]   = FMaterialTextureRoute(EMaterialTextureSlot::BaseColor, ETextureChannel::A);
    }

    FFloatColor           Albedo;
    float                 Roughness;
    float                 Metallic;
    float                 AmbientOcclusion;
    float                 ParallaxHeightScale;
    float                 ParallaxMinLayers;
    float                 ParallaxMaxLayers;
    EMaterialFlags        MaterialFlags;
    FMaterialTextureRoute Routes[EMaterialScalar::Count];
};

// Mirrored by the NORMAL_MAP_FLAG_* defines in Assets/Shaders/Structs.hlsli
enum class ENormalMapFlags : uint32
{
    None       = 0,
    Enabled    = (1u << 0), // A real normal map is bound, rather than the fallback flat one
    PositiveY  = (1u << 1), // Green has to be flipped on the way in to reach the engine's basis
    TwoChannel = (1u << 2), // RG only, Z reconstructed (BC5); otherwise RGB is stored outright
};

ENUM_CLASS_OPERATORS(ENormalMapFlags);

struct FMaterialHLSL
{
    // 0-16 
    Vector3 Albedo    = Vector3(1.0f);
    float   Roughness = 1.0f;
    
    // 16-32
    float Metallic            = 0.0f;
    float AmbientOcclusion    = 1.0f;
    float ParallaxHeightScale = 0.03f;
    float ParallaxMinLayers   = 32.0f;

    // 32-48
    float           ParallaxMaxLayers = 64.0f;
    uint32          ScalarRoutes      = 0; // Roughness | Metallic << 8 | Occlusion << 16 | Opacity << 24
    ENormalMapFlags NormalMapFlags    = ENormalMapFlags::None;
    uint32          Padding0          = 0;

    // 48-80
    FRHIDescriptorHandle SlotHandles[EMaterialTextureSlot::Count] = {};
    FRHIDescriptorHandle SamplerHandle                                    = {};
};

static_assert(sizeof(FRHIDescriptorHandle) == sizeof(uint32), "FRHIDescriptorHandle must be 4 bytes for the HLSL Material layout");
static_assert(sizeof(FMaterialHLSL) == 80, "FMaterialHLSL must match the HLSL Material layout");

class ENGINE_API FMaterial
{
public:
    static EMaterialFlags GetSupportedMaterialFlags(const FVertexDeclaration& Declaration);

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

    bool HasAlphaMask()         const { return IsEnumFlagSet(MaterialInfo.MaterialFlags, EMaterialFlags::EnableAlpha) && IsRouteFed(EMaterialScalar::Opacity); }
    bool HasHeightMap()         const { return IsEnumFlagSet(MaterialInfo.MaterialFlags, EMaterialFlags::EnableHeight) && GetTexture(EMaterialTextureSlot::Height).IsValid(); }
    bool HasNormalMap()         const { return IsEnumFlagSet(MaterialInfo.MaterialFlags, EMaterialFlags::EnableNormalMapping); }
    bool IsNormalMapPositiveY() const { return IsEnumFlagSet(MaterialInfo.MaterialFlags, EMaterialFlags::NormalMapPositiveY); }
    bool HasParallaxClipping()  const { return IsEnumFlagSet(MaterialInfo.MaterialFlags, EMaterialFlags::EnableParallaxClipping) && HasHeightMap(); }

    bool IsDoubleSided() const { return IsEnumFlagSet(MaterialInfo.MaterialFlags, EMaterialFlags::DoubleSided); }

    bool ShouldRenderInForwardPass() const { return IsEnumFlagSet(MaterialInfo.MaterialFlags, EMaterialFlags::ForceForwardPass); }
    bool ShouldRenderInPrePass()     const { return !ShouldRenderInForwardPass(); }

    bool SupportsPixelDiscard() const { return HasHeightMap() || HasAlphaMask(); }

    void SetRoughnessRoute(const FMaterialTextureRoute& Route) { SetRoute(EMaterialScalar::Roughness, Route); }
    void SetMetallicRoute(const FMaterialTextureRoute& Route)  { SetRoute(EMaterialScalar::Metallic, Route); }
    void SetOcclusionRoute(const FMaterialTextureRoute& Route) { SetRoute(EMaterialScalar::Occlusion, Route); }
    void SetOpacityRoute(const FMaterialTextureRoute& Route)   { SetRoute(EMaterialScalar::Opacity, Route); }

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
        if (!GetTexture(EMaterialTextureSlot::Height).IsValid())
        {
            Flags &= ~(EMaterialFlags::EnableHeight | EMaterialFlags::EnableParallaxClipping);
        }

        if (!HasAlphaMask())
        {
            Flags &= ~EMaterialFlags::EnableAlpha;
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

    FRHITextureRef& GetTexture(EMaterialTextureSlot::Type Slot)
    {
        return Textures[Slot];
    }

    const FRHITextureRef& GetTexture(EMaterialTextureSlot::Type Slot) const
    {
        return Textures[Slot];
    }

    void SetTexture(EMaterialTextureSlot::Type Slot, const FRHITextureRef& InTexture)
    {
        Textures[Slot] = InTexture;
    }

    const FMaterialTextureRoute& GetRoute(EMaterialScalar::Type Scalar) const
    {
        return MaterialInfo.Routes[Scalar];
    }

    bool IsRouteFed(EMaterialScalar::Type Scalar) const
    {
        const FMaterialTextureRoute& Route = GetRoute(Scalar);
        return Route.IsRouted() && GetTexture(Route.Slot).IsValid();
    }

    void SetRoute(EMaterialScalar::Type Scalar, const FMaterialTextureRoute& Route)
    {
        MaterialInfo.Routes[Scalar] = Route;
    }

private:
    FRHITextureRef      Textures[EMaterialTextureSlot::Count];
    String              Name;
    FMaterialInfo       MaterialInfo;
    int32               BufferIndex = 0;
    FRHISamplerStateRef Sampler;
};

class FMaterialMaskSlots
{
public:
    explicit FMaterialMaskSlots(FMaterial& InMaterial)
        : Material(InMaterial)
        , Next(EMaterialTextureSlot::MaskA)
    {
    }

    EMaterialTextureSlot::Type Assign(const FRHITextureRef& InTexture)
    {
        if (!InTexture)
        {
            return EMaterialTextureSlot::Count;
        }

        for (uint32 Slot = EMaterialTextureSlot::MaskA; Slot < Next; ++Slot)
        {
            if (Material.GetTexture(EMaterialTextureSlot::Type(Slot)).Get() == InTexture.Get())
            {
                return EMaterialTextureSlot::Type(Slot);
            }
        }

        CHECK(Next < EMaterialTextureSlot::Count);

        const EMaterialTextureSlot::Type Slot = Next;
        Material.SetTexture(Slot, InTexture);

        Next = EMaterialTextureSlot::Type(Next + 1);
        return Slot;
    }

private:
    FMaterial&                 Material;
    EMaterialTextureSlot::Type Next;
};
