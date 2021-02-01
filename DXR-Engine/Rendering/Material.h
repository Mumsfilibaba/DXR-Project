#pragma once
#include "RenderLayer/Resources.h"
#include "RenderLayer/ResourceHelpers.h"

#include <Containers/TStaticArray.h>

struct MaterialProperties
{
    XMFLOAT3 Albedo    = XMFLOAT3(1.0f, 1.0f, 1.0f);
    Float Roughness    = 0.0f;
    Float Metallic     = 0.0f;
    Float AO           = 0.5f;
    Int32 EnableHeight = 0;
};

class Material
{
public:
    Material(const MaterialProperties& InProperties);
    ~Material() = default;

    void Init();

    void BuildBuffer(class CommandList& CmdList);

    FORCEINLINE Bool IsBufferDirty() const
    {
        return MaterialBufferIsDirty;
    }

    void SetAlbedo(const XMFLOAT3& Albedo);
    void SetAlbedo(Float R, Float G, Float B);

    void SetMetallic(Float Metallic);
    void SetRoughness(Float Roughness);
    void SetAmbientOcclusion(Float AO);

    void EnableHeightMap(Bool EnableHeightMap);

    void SetDebugName(const std::string& InDebugName);

    // ShaderResourceView are sorted in the way that the deferred rendering pass wants them
    // This means that one can call BindShaderResourceViews directly with this function
    ShaderResourceView* const* GetShaderResourceViews() const;

    FORCEINLINE SamplerState* GetMaterialSampler() const
    {
        return Sampler.Get();
    }

    FORCEINLINE ConstantBuffer* GetMaterialBuffer() const
    {
        return MaterialBuffer.Get();
    }

    FORCEINLINE Bool HasAlphaMask() const
    {
        return AlphaMask;
    }

    FORCEINLINE Bool HasHeightMap() const
    {
        return HeightMap;
    }

    FORCEINLINE const MaterialProperties& GetMaterialProperties() const 
    {
        return Properties;
    }

public:
    SampledTexture2D AlbedoMap;
    SampledTexture2D NormalMap;
    SampledTexture2D RoughnessMap;
    SampledTexture2D HeightMap;
    SampledTexture2D AOMap;
    SampledTexture2D MetallicMap;
    SampledTexture2D AlphaMask;

private:
    std::string	DebugName;
    Bool MaterialBufferIsDirty = true;
    
    MaterialProperties         Properties;
    TSharedRef<ConstantBuffer> MaterialBuffer;
    TSharedRef<SamplerState>   Sampler;

    mutable TStaticArray<ShaderResourceView*, 7> ShaderResourceViews;
};