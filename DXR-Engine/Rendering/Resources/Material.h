#pragma once
#include "CoreRHI/RHIResources.h"

#include "Core/Containers/StaticArray.h"

#include "Core/Math/Vector3.h"

struct SMaterialDesc
{
    CVector3 Albedo = CVector3( 1.0f );
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

    void BuildBuffer( class CRHICommandList& CmdList );

    FORCEINLINE bool IsBufferDirty() const
    {
        return MaterialBufferIsDirty;
    }

    void SetAlbedo( const CVector3& Albedo );
    void SetAlbedo( float r, float g, float b );

    void SetMetallic( float Metallic );
    void SetRoughness( float Roughness );
    void SetAmbientOcclusion( float AO );

    void ForceForwardPass( bool ForceForwardRender );

    void EnableHeightMap( bool InEnableHeightMap );
    void EnableAlphaMask( bool InEnableAlphaMask );

    void SetDebugName( const CString& InDebugName );

    // ShaderResourceView are sorted in the way that the deferred rendering pass wants them
    // This means that one can call BindShaderResourceViews directly with this function
    CRHIShaderResourceView* const* GetShaderResourceViews() const;

    FORCEINLINE CRHISamplerState* GetMaterialSampler() const
    {
        return Sampler.Get();
    }

    FORCEINLINE CRHIConstantBuffer* GetMaterialBuffer() const
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

    FORCEINLINE const SMaterialDesc& GetMaterialProperties() const
    {
        return Properties;
    }

public:
    TSharedRef<CRHITexture2D> AlbedoMap;
    TSharedRef<CRHITexture2D> NormalMap;
    TSharedRef<CRHITexture2D> RoughnessMap;
    TSharedRef<CRHITexture2D> HeightMap;
    TSharedRef<CRHITexture2D> AOMap;
    TSharedRef<CRHITexture2D> MetallicMap;
    TSharedRef<CRHITexture2D> AlphaMask;

private:
    CString DebugName;

    bool MaterialBufferIsDirty = true;

    bool RenderInForwardPass = false;

    SMaterialDesc        	   Properties;
    TSharedRef<CRHIConstantBuffer> MaterialBuffer;
    TSharedRef<CRHISamplerState>   Sampler;

    mutable TStaticArray<CRHIShaderResourceView*, 7> ShaderResourceViews;
};
