#pragma once
#include "Engine/EngineModule.h"

#include "RHI/RHIResources.h"

#include "Core/Math/Vector3.h"
#include "Core/Containers/StaticArray.h"

#define SafeGetDefaultSRV(Texture) (Texture ? Texture->GetShaderResourceView() : nullptr)

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// FMaterialDesc

struct FMaterialDesc
{
    FVector3 Albedo = FVector3(1.0f);
    float Roughness = 0.0f;

    float Metallic     = 0.0f;
    float AO           = 0.5f;
    int32 EnableHeight = 0;
    int32 EnableMask   = 0;
};

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// FMaterial

class ENGINE_API FMaterial
{
public:
    FMaterial(const FMaterialDesc& InProperties);
    ~FMaterial() = default;

    void Initialize();

    void BuildBuffer(class FRHICommandList& CommandList);

    FORCEINLINE bool IsBufferDirty() const { return bMaterialBufferIsDirty; }

    void SetAlbedo(const FVector3& Albedo);
    void SetAlbedo(float r, float g, float b);

    void SetMetallic(float Metallic);
    void SetRoughness(float Roughness);
    void SetAmbientOcclusion(float AO);

    void ForceForwardPass(bool bForceForwardRender);

    void EnableHeightMap(bool bInEnableHeightMap);
    void EnableAlphaMask(bool bInEnableAlphaMask);

    void SetDebugName(const FString& InDebugName);

    // ShaderResourceView are sorted in the way that the deferred rendering pass wants them
    // This means that one can call BindShaderResourceViews directly with this function
    FRHIShaderResourceView* const* GetShaderResourceViews() const;

    FORCEINLINE FRHISamplerState* GetMaterialSampler() const
    {
        return Sampler.Get();
    }

    FORCEINLINE FRHIConstantBuffer* GetMaterialBuffer() const
    {
        return MaterialBuffer.Get();
    }

    FORCEINLINE bool ShouldRenderInPrePass()
    {
        return !HasAlphaMask() && !HasHeightMap() && !bRenderInForwardPass;
    }

    FORCEINLINE bool ShouldRenderInForwardPass()
    {
        return bRenderInForwardPass;
    }

    FORCEINLINE bool HasAlphaMask() const
    {
        return AlphaMask;
    }

    FORCEINLINE bool HasHeightMap() const
    {
        return HeightMap;
    }

    FORCEINLINE const FMaterialDesc& GetMaterialProperties() const
    {
        return Properties;
    }

public:
    FRHITexture2DRef AlbedoMap;
    FRHITexture2DRef NormalMap;
    FRHITexture2DRef RoughnessMap;
    FRHITexture2DRef HeightMap;
    FRHITexture2DRef AOMap;
    FRHITexture2DRef MetallicMap;
    FRHITexture2DRef AlphaMask;

private:
    FString DebugName;

    bool bMaterialBufferIsDirty = true;
    bool bRenderInForwardPass   = false;

    FMaterialDesc        	       Properties;
    FRHIConstantBufferRef MaterialBuffer;
    FRHISamplerStateRef   Sampler;

    mutable TStaticArray<FRHIShaderResourceView*, 7> ShaderResourceViews;
};
