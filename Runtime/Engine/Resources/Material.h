#pragma once
#include "Engine/EngineModule.h"
#include "Core/Math/Vector3.h"
#include "Core/Containers/StaticArray.h"
#include "RHI/RHIResources.h"

#define SafeGetDefaultSRV(Texture) (Texture ? Texture->GetShaderResourceView() : nullptr)

// TODO: This should be refactored into using different shaders
enum EAlphaMaskMode
{
    AlphaMaskMode_Disabled        = 0, // Disabled completely
    AlphaMaskMode_Enabled         = 1, // Stored in separate texture
    AlphaMaskMode_DiffuseCombined = 2, // Stored in Alpha channel of diffuse texture
};


struct FMaterialDesc
{
    FVector3 Albedo       = FVector3(1.0f);
    float    Roughness    = 0.0f;

    float    Metallic     = 0.0f;
    float    AO           = 0.5f;
    int32    EnableHeight = 0;
    int32    EnableMask   = AlphaMaskMode_Disabled;
};


class ENGINE_API FMaterial
{
public:
    FMaterial(const FMaterialDesc& InProperties);
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

    void EnableHeightMap(bool bInEnableHeightMap);
    
    void EnableAlphaMask(bool bInEnableAlphaMask);

    void EnableDoubleSided(bool bInIsDoubleSided)
    {
        bIsDoubleSided = bInIsDoubleSided;
    }

    void SetDebugName(const FString& InDebugName);

    bool HasAlphaMask() const
    {
        return AlphaMask && Properties.EnableMask || Properties.EnableMask == AlphaMaskMode_DiffuseCombined;
    }

    bool HasHeightMap() const
    {
        return HeightMap && Properties.EnableHeight == 1;
    }

    bool IsDoubleSided() const
    {
        return bIsDoubleSided;
    }

    bool IsPackedMaterial() const
    {
        return Properties.EnableMask == AlphaMaskMode_DiffuseCombined || SpecularMap != nullptr;
    }

    FRHISamplerState* GetMaterialSampler() const
    {
        return Sampler.Get();
    }

    FRHIBuffer* GetMaterialBuffer() const
    {
        return MaterialBuffer.Get();
    }

    bool ShouldRenderInPrePass()
    {
        return !bRenderInForwardPass;
    }

    bool ShouldRenderInForwardPass()
    {
        return bRenderInForwardPass;
    }

    const FMaterialDesc& GetMaterialProperties() const
    {
        return Properties;
    }

    FRHIShaderResourceView* GetAlphaMaskSRV() const
    {
        return Properties.EnableMask == AlphaMaskMode_DiffuseCombined ? AlbedoMap->GetShaderResourceView() : AlphaMask->GetShaderResourceView();
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
    FMaterialDesc       Properties;
    FRHIBufferRef       MaterialBuffer;
    FRHISamplerStateRef Sampler;

    bool bRenderInForwardPass   = false;
    bool bIsDoubleSided         = false;
    bool bMaterialBufferIsDirty = true;

    FString DebugName;
};
