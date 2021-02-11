#pragma once
#include "RenderLayer/Resources.h"

#include <Containers/StaticArray.h>

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

    SamplerState* GetMaterialSampler() const { return Sampler.Get(); }
    ConstantBuffer* GetMaterialBuffer() const { return MaterialBuffer.Get(); }

    Bool HasAlphaMask() const { return AlphaMask; }

    Bool HasHeightMap() const { return HeightMap; }

    const MaterialProperties& GetMaterialProperties() const { return Properties; }

public:
    TRef<Texture2D> AlbedoMap;
    TRef<Texture2D> NormalMap;
    TRef<Texture2D> RoughnessMap;
    TRef<Texture2D> HeightMap;
    TRef<Texture2D> AOMap;
    TRef<Texture2D> MetallicMap;
    TRef<Texture2D> AlphaMask;

private:
    std::string	DebugName;
    Bool MaterialBufferIsDirty = true;
    
    MaterialProperties         Properties;
    TRef<ConstantBuffer> MaterialBuffer;
    TRef<SamplerState>   Sampler;

    mutable TStaticArray<ShaderResourceView*, 7> ShaderResourceViews;
};