#pragma once
#include "RenderLayer/Resources.h"

#include "Core/Containers/StaticArray.h"

#include "Math/Vector3.h"

struct SMaterialDesc
{
    CVector3 Albedo = CVector3( 1.0f);
    float Roughness = 0.0f;

    float Metallic = 0.0f;
    float AO = 0.5f;
    int32 EnableHeight = 0;
    int32 EnableMask = 0;
};

class CMaterial
{
public:
    CMaterial( const SMaterialDesc& InProperties );
    ~CMaterial() = default;

    void Init();

    void BuildBuffer( class CommandList& CmdList );

    FORCEINLINE bool IsBufferDirty() const
    {
        return MaterialBufferIsDirty;
    }

    void SetAlbedo( const CVector3& Albedo );
    void SetAlbedo( float R, float G, float B );

    void SetMetallic( float Metallic );
    void SetRoughness( float Roughness );
    void SetAmbientOcclusion( float AO );

    void ForceForwardPass( bool ForceForwardRender );

    void EnableHeightMap( bool InEnableHeightMap );
    void EnableAlphaMask( bool InEnableAlphaMask );

    void SetDebugName( const std::string& InDebugName );

    // ShaderResourceView are sorted in the way that the deferred rendering pass wants them
    // This means that one can call BindShaderResourceViews directly with this function
    ShaderResourceView* const* GetShaderResourceViews() const;

    SamplerState* GetMaterialSampler() const
    {
        return Sampler.Get();
    }
    ConstantBuffer* GetMaterialBuffer() const
    {
        return MaterialBuffer.Get();
    }

    FORCEINLINE bool ShouldRenderInPrePass()
    {
        return !HasAlphaMask() && !HasHeightMap() && !RenderInForwardPass;
    }
    FORCEINLINE bool ShouldRenderInForwardPass()
    {
        return RenderInForwardPass;
    }

    FORCEINLINE bool HasAlphaMask() const
    {
        return AlphaMask;
    }
    FORCEINLINE bool HasHeightMap() const
    {
        return HeightMap;
    }

    const SMaterialDesc& GetMaterialProperties() const
    {
        return Properties;
    }

public:
    TRef<Texture2D> AlbedoMap;
    TRef<Texture2D> NormalMap;
    TRef<Texture2D> RoughnessMap;
    TRef<Texture2D> HeightMap;
    TRef<Texture2D> AOMap;
    TRef<Texture2D> MetallicMap;
    TRef<Texture2D> AlphaMask;

private:
    std::string DebugName;

    bool MaterialBufferIsDirty = true;

    bool RenderInForwardPass = false;

    SMaterialDesc        Properties;
    TRef<ConstantBuffer> MaterialBuffer;
    TRef<SamplerState>   Sampler;

    mutable TStaticArray<ShaderResourceView*, 7> ShaderResourceViews;
};