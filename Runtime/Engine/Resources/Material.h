#pragma once
#include "Engine/EngineModule.h"
#include "Core/Math/Vector3.h"
#include "Core/Containers/StaticArray.h"
#include "RHI/RHIResources.h"

#define SafeGetDefaultSRV(Texture) (Texture ? Texture->GetShaderResourceView() : nullptr)

enum class EMaterialFlags : int32
{
    None                = 0,       // No flags
    EnableHeight        = FLAG(0), // Enable HeightMaps (Parallax Occlusion Mapping)
    EnableAlpha         = FLAG(1), // Enable Alpha Textures
    EnableNormalMapping = FLAG(2), // Enable Normal Mapping
    PackedDiffuseAlpha  = FLAG(3), // The alpha and diffuse is stored in the same texture
    PackedParams        = FLAG(4), // The Roughness, AO, and Metallic is stored in the same texture
    DoubleSided         = FLAG(5), // The Material should be rendered without culling
    ForceForwardPass    = FLAG(6), // This material should be rendered in the ForwardPass
};

ENUM_CLASS_OPERATORS(EMaterialFlags);

struct FMaterialInfo
{
    FMaterialInfo()
        : Albedo(1.0f)
        , Roughness(0.0f)
        , Metallic(0.0f)
        , AmbientOcclusion(0.5f)
        , MaterialFlags(EMaterialFlags::None)

    {
    }

    FVector3       Albedo;
    float          Roughness;
    float          Metallic;
    float          AmbientOcclusion;
    EMaterialFlags MaterialFlags;
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
    FMaterial(const FMaterialInfo& InMaterialInfo);
    ~FMaterial();

    void Initialize();
    void BuildBuffer(class FRHICommandList& CommandList);

    bool IsBufferDirty() const { return bMaterialBufferIsDirty; }

    void SetAlbedo(const FVector3& Albedo);
    void SetAlbedo(float r, float g, float b);
    void SetMetallic(float Metallic);
    void SetRoughness(float Roughness);
    void SetAmbientOcclusion(float AO);
    void SetMaterialFlags(EMaterialFlags InFlags, bool bUpdateOnly = false);
    
    void ForceForwardPass(bool bForceForwardRender);
    
    void EnableHeightMap(bool bEnableHeightMap);
    void EnableAlphaMask(bool bEnableAlphaMask);
    void EnableDoubleSided(bool bIsDoubleSided);
    
    void SetName(const FString& InName);

    bool HasAlphaMask()          const { return (MaterialInfo.MaterialFlags & EMaterialFlags::EnableAlpha) != EMaterialFlags::None; }
    bool HasHeightMap()          const { return (MaterialInfo.MaterialFlags & EMaterialFlags::EnableHeight) != EMaterialFlags::None; }
    bool HasNormalMap()          const { return (MaterialInfo.MaterialFlags & EMaterialFlags::EnableNormalMapping) != EMaterialFlags::None; }
    bool HasPackedDiffuseAlpha() const { return (MaterialInfo.MaterialFlags & EMaterialFlags::PackedDiffuseAlpha) != EMaterialFlags::None; }

    bool IsDoubleSided()    const { return (MaterialInfo.MaterialFlags & EMaterialFlags::DoubleSided) != EMaterialFlags::None; }
    bool IsPackedMaterial() const { return (MaterialInfo.MaterialFlags & (EMaterialFlags::PackedDiffuseAlpha | EMaterialFlags::PackedParams)) != EMaterialFlags::None; }
    
    bool ShouldRenderInForwardPass() const { return (MaterialInfo.MaterialFlags & EMaterialFlags::ForceForwardPass) != EMaterialFlags::None; }
    bool ShouldRenderInPrePass()     const { return !ShouldRenderInForwardPass(); }

    bool SupportsPixelDiscard() const { return (MaterialInfo.MaterialFlags & (EMaterialFlags::EnableHeight | EMaterialFlags::EnableAlpha)) != EMaterialFlags::None; }

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
        return (MaterialInfo.MaterialFlags & EMaterialFlags::PackedDiffuseAlpha) != EMaterialFlags::None ? AlbedoMap->GetShaderResourceView() : AlphaMask->GetShaderResourceView();
    }

    EMaterialFlags GetMaterialFlags() const 
    {
        return MaterialInfo.MaterialFlags;
    }

    const FMaterialInfo& GetMaterialInfo() const
    {
        return MaterialInfo;
    }
    
    const FString& GetName() const
    {
        return Name;
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
    FString             Name;
    FMaterialHLSL       MaterialData;
    FMaterialInfo       MaterialInfo;
    bool                bMaterialBufferIsDirty;
    FRHIBufferRef       MaterialBuffer;
    FRHISamplerStateRef Sampler;
};
