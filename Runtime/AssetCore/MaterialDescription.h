#pragma once
#include "Core/Core.h"
#include "Core/Math/Color.h"
#include "Core/Templates/Utility/EnumOperators.h"

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
    Translucent             = FLAG(7), // Alpha-blended in the ForwardPass instead of written to the GBuffer
    EnableRefraction        = FLAG(8), // Translucent surfaces sample the skybox along the refracted view vector
};

ENUM_CLASS_OPERATORS(EMaterialFlags);

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

    bool operator==(const FMaterialTextureRoute& Other) const
    {
        return Slot == Other.Slot && Channel == Other.Channel && bInvert == Other.bInvert;
    }

    EMaterialTextureSlot::Type Slot    = EMaterialTextureSlot::Count;
    ETextureChannel            Channel = ETextureChannel::R;
    bool                       bInvert = false; // Lets a gloss map feed roughness
};

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
        , Opacity(1.0f)
        , IndexOfRefraction(1.5f)
        , RefractionStrength(1.0f)
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
    float                 Opacity;
    float                 IndexOfRefraction;
    float                 RefractionStrength;
    EMaterialFlags        MaterialFlags;
    FMaterialTextureRoute Routes[EMaterialScalar::Count];
};
