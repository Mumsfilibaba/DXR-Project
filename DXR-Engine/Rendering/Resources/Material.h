#pragma once
#include "RenderLayer/Resources.h"

#include <Containers/StaticArray.h>

struct MaterialProperties
{
    XMFLOAT3 Diffuse = XMFLOAT3(1.0f, 1.0f, 1.0f);

    Float Roughness = 0.0f;
    Float Metallic  = 0.0f;
    Float AO        = 0.5f;

    Int32 EnableHeight   = 0;
    Int32 EnableEmissive = 0;
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

    void SetDiffuse(const XMFLOAT3& Albedo);
    void SetDiffuse(Float r, Float g, Float b);

    void SetMetallic(Float Metallic);
    void SetRoughness(Float Roughness);
    void SetAmbientOcclusion(Float AO);

    void EnableHeightMap(Bool EnableHeightMap);
    void EnableEmissiveMap(Bool EnableEmissiveMap);

    void EnableTransparency(Bool EnableTransparency);

    void SetDebugName(const std::string& InDebugName);

    // ShaderResourceView are sorted in the way that the deferred rendering pass wants them
    // This means that one can call BindShaderResourceViews directly with this function
    ShaderResourceView* const* GetShaderResourceViews() const;

    SamplerState* GetMaterialSampler() const { return Sampler.Get(); }
    ConstantBuffer* GetMaterialBuffer() const { return MaterialBuffer.Get(); }

    Bool HasHeightMap() const { return Properties.EnableHeight == 1; }
    Bool HasEmissiveMap() const { return Properties.EnableEmissive == 1; }
    Bool HasTransparency() const { return TransparencyEnabled; }

    const MaterialProperties& GetMaterialProperties() const { return Properties; }

public:
    TRef<Texture2D> DiffuseMap;  // Diffuse, Alpha
    TRef<Texture2D> NormalMap;   // Normal map
    TRef<Texture2D> EmissiveMap; // Emissive
    TRef<Texture2D> SpecularMap; // AO, Roughness, Metallic
    TRef<Texture2D> HeightMap;   // Height

private:
    std::string	DebugName;
    
    Bool MaterialBufferIsDirty = true;
    
    Bool TransparencyEnabled = false;

    MaterialProperties Properties;

    TRef<ConstantBuffer> MaterialBuffer;
    TRef<SamplerState>   Sampler;

    mutable TStaticArray<ShaderResourceView*, 5> ShaderResourceViews;
};