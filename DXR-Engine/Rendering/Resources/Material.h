#pragma once
#include "RenderLayer/Resources.h"

#include "Core/Containers/StaticArray.h"

struct SMaterialDesc
{
    XMFLOAT3 Albedo = XMFLOAT3( 1.0f, 1.0f, 1.0f );
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
        return m_MaterialBufferIsDirty;
    }

    void SetAlbedo( const XMFLOAT3& Albedo );
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
        return m_Sampler.Get();
    }
    ConstantBuffer* GetMaterialBuffer() const
    {
        return m_MaterialBuffer.Get();
    }

    FORCEINLINE bool ShouldRenderInPrePass()
    {
        return !HasAlphaMask() && !HasHeightMap() && !m_RenderInForwardPass;
    }
    FORCEINLINE bool ShouldRenderInForwardPass()
    {
        return m_RenderInForwardPass;
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
        return m_Properties;
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
    std::string m_DebugName;

    bool m_MaterialBufferIsDirty = true;

    bool m_RenderInForwardPass = false;

    SMaterialDesc        m_Properties;
    TRef<ConstantBuffer> m_MaterialBuffer;
    TRef<SamplerState>   m_Sampler;

    mutable TStaticArray<ShaderResourceView*, 7> m_ShaderResourceViews;
};