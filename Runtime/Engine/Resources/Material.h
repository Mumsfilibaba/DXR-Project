#pragma once
#include "Engine/EngineModule.h"
#include "Core/Math/Vector3.h"
#include "Core/Containers/StaticArray.h"
#include "RHI/RHIResources.h"

#define SafeGetDefaultSRV(Texture) (Texture ? Texture->GetShaderResourceView() : nullptr)

enum EMaterialFlags : int32
{
    MaterialFlag_None                = 0,       // No flags
    MaterialFlag_EnableHeight        = FLAG(0), // Enable HeightMaps (Parallax Occlusion Mapping)
    MaterialFlag_EnableAlpha         = FLAG(1), // Enable Alpha Textures
    MaterialFlag_EnableNormalMapping = FLAG(2), // Enable Normal Mapping
    MaterialFlag_PackedDiffuseAlpha  = FLAG(3), // The alpha and diffuse is stored in the same texture
    MaterialFlag_PackedParams        = FLAG(4), // The Roughness, AO, and Metallic is stored in the same texture
    MaterialFlag_DoubleSided         = FLAG(5), // The Material should be rendered without culling
    MaterialFlag_ForceForwardPass    = FLAG(6), // This material should be rendered in the ForwardPass
};

ENUM_CLASS_OPERATORS(EMaterialFlags);

struct FMaterialCreateInfo
{
    FMaterialCreateInfo()
        : Albedo(1.0f)
        , Roughness(0.0f)
        , Metallic(0.0f)
        , AmbientOcclusion(0.5f)
        , MaterialFlags(MaterialFlag_None)

    {
    }

    EMaterialFlags MaterialFlags;

    FVector3       Albedo;
    float          Roughness;

    float          Metallic;
    float          AmbientOcclusion;
};

struct FMaterialHLSL
{
    FVector3 Albedo           = FVector3(1.0f);
    float    Roughness        = 1.0f;

    float    Metallic         = 0.0f;
    float    AmbientOcclusion = 1.0f;
    int32    Padding0         = 0;
    int32    Padding1         = 0;
};

class ENGINE_API FMaterial
{
public:
    FMaterial(const FMaterialCreateInfo& InProperties);
    ~FMaterial() = default;

    void Initialize();
    void BuildBuffer(class FRHICommandList& CommandList);

    bool IsBufferDirty() const { return bMaterialBufferIsDirty; }

    void SetAlbedo(const FVector3& Albedo);
    void SetAlbedo(float r, float g, float b);
    void SetMetallic(float Metallic);
    void SetRoughness(float Roughness);
    void SetAmbientOcclusion(float AO);

    void ForceForwardPass(bool bForceForwardRender);
    void EnableHeightMap(bool bEnableHeightMap);
    void EnableAlphaMask(bool bEnableAlphaMask);
    void EnableDoubleSided(bool bIsDoubleSided);
    void SetDebugName(const FString& InDebugName);

    bool HasAlphaMask() const { return (Properties.MaterialFlags & MaterialFlag_EnableAlpha) != MaterialFlag_None; }
    bool HasHeightMap() const { return (Properties.MaterialFlags & MaterialFlag_EnableHeight) != MaterialFlag_None; }
    bool HasNormalMap() const { return (Properties.MaterialFlags & MaterialFlag_EnableNormalMapping) != MaterialFlag_None; }

    bool IsDoubleSided()    const { return (Properties.MaterialFlags & MaterialFlag_DoubleSided) != MaterialFlag_None; }
    bool IsPackedMaterial() const { return (Properties.MaterialFlags & (MaterialFlag_PackedDiffuseAlpha | MaterialFlag_PackedParams)) != MaterialFlag_None; }

    bool ShouldRenderInPrePass()     const { return (Properties.MaterialFlags & MaterialFlag_ForceForwardPass) == MaterialFlag_None; }
    bool ShouldRenderInForwardPass() const { return (Properties.MaterialFlags & MaterialFlag_ForceForwardPass) != MaterialFlag_None; }

    FRHISamplerState* GetMaterialSampler() const
    {
        return Sampler.Get();
    }

    FRHIBuffer* GetMaterialBuffer() const
    {
        return MaterialBuffer.Get();
    }

    FRHIShaderResourceView* GetAlphaMaskSRV() const
    {
        return (Properties.MaterialFlags & MaterialFlag_PackedDiffuseAlpha) != MaterialFlag_None ? AlbedoMap->GetShaderResourceView() : AlphaMask->GetShaderResourceView();
    }

    EMaterialFlags GetMaterialFlags() const 
    {
        return Properties.MaterialFlags;
    }

public:
    FRHITextureRef AlbedoMap;
    FRHITextureRef NormalMap;
    FRHITextureRef RoughnessMap;
    FRHITextureRef HeightMap;
    FRHITextureRef AOMap;
    FRHITextureRef SpecularMap;
    FRHITextureRef MetallicMap;
    FRHITextureRef AlphaMask;

private:
    FMaterialHLSL       MaterialData;
    FMaterialCreateInfo Properties;
    bool                bMaterialBufferIsDirty;

    FRHIBufferRef       MaterialBuffer;
    FRHISamplerStateRef Sampler;

    FString             DebugName;
};
