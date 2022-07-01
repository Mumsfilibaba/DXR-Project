#pragma once
#include "Engine/EngineModule.h"

#include "RHI/RHIResources.h"

#include "Core/Math/Vector3.h"
#include "Core/Containers/StaticArray.h"

#define SafeGetDefaultSRV(Texture) (Texture ? Texture->GetShaderResourceView() : nullptr)

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// SMaterialDesc

struct SMaterialDesc
{
    CVector3 Albedo = CVector3(1.0f);
    float Roughness = 0.0f;

    float Metallic     = 0.0f;
    float AO           = 0.5f;
    int32 EnableHeight = 0;
    int32 EnableMask   = 0;
};

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// CMaterial

class ENGINE_API CMaterial
{
public:
    CMaterial(const SMaterialDesc& InProperties);
    ~CMaterial() = default;

    void Init();

    void BuildBuffer(class FRHICommandList& CmdList);

    FORCEINLINE bool IsBufferDirty() const { return bMaterialBufferIsDirty; }

    void SetAlbedo(const CVector3& Albedo);
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

    FORCEINLINE const SMaterialDesc& GetMaterialProperties() const
    {
        return Properties;
    }

public:
    TSharedRef<FRHITexture2D> AlbedoMap;
    TSharedRef<FRHITexture2D> NormalMap;
    TSharedRef<FRHITexture2D> RoughnessMap;
    TSharedRef<FRHITexture2D> HeightMap;
    TSharedRef<FRHITexture2D> AOMap;
    TSharedRef<FRHITexture2D> MetallicMap;
    TSharedRef<FRHITexture2D> AlphaMask;

private:
    FString DebugName;

    bool bMaterialBufferIsDirty = true;
    bool bRenderInForwardPass   = false;

    SMaterialDesc        	       Properties;
    TSharedRef<FRHIConstantBuffer> MaterialBuffer;
    TSharedRef<FRHISamplerState>   Sampler;

    mutable TStaticArray<FRHIShaderResourceView*, 7> ShaderResourceViews;
};
