#pragma once
#include "RHIResourceBase.h"

enum ETextureFlags
{
    TextureFlag_None = 0,
    TextureFlag_RTV = FLAG( 1 ), // RenderTargetView
    TextureFlag_DSV = FLAG( 2 ), // DepthStencilView
    TextureFlag_UAV = FLAG( 3 ), // UnorderedAccessView
    TextureFlag_SRV = FLAG( 4 ), // ShaderResourceView
    TextureFlag_NoDefaultRTV = FLAG( 5 ), // Do not create default RenderTargetView
    TextureFlag_NoDefaultDSV = FLAG( 6 ), // Do not create default DepthStencilView
    TextureFlag_NoDefaultUAV = FLAG( 7 ), // Do not create default UnorderedAccessView
    TextureFlag_NoDefaultSRV = FLAG( 8 ), // Do not create default ShaderResourceView
    TextureFlags_RWTexture = TextureFlag_UAV | TextureFlag_SRV,
    TextureFlags_RenderTarget = TextureFlag_RTV | TextureFlag_SRV,
    TextureFlags_ShadowMap = TextureFlag_DSV | TextureFlag_SRV,
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class CRHITexture : public CRHIResource
{
public:

    CRHITexture( EFormat InFormat, uint32 InNumMips, uint32 InFlags, const SClearValue& InOptimalClearValue )
        : CRHIResource()
        , Format( InFormat )
        , NumMips( InNumMips )
        , Flags( InFlags )
        , OptimalClearValue( InOptimalClearValue )
    {
    }

    /* Cast to Texture2D */
    virtual class CRHITexture2D* AsTexture2D() { return nullptr; }
    /* Cast to Texture2DArray */
    virtual class CRHITexture2DArray* AsTexture2DArray() { return nullptr; }
    /* Cast to TextureCube */
    virtual class CRHITextureCube* AsTextureCube() { return nullptr; }
    /* Cast to TextureCubeArray */
    virtual class CRHITextureCubeArray* AsTextureCubeArray() { return nullptr; }
    /* Cast to Texture3D */
    virtual class CRHITexture3D* AsTexture3D() { return nullptr; }

    /* Returns a ShaderResourceView of the full resource if texture is created with TextureFlag_SRV */
    virtual class CRHIShaderResourceView* GetShaderResourceView() const { return nullptr; }

    FORCEINLINE EFormat GetFormat() const
    {
        return Format;
    }

    FORCEINLINE uint32 GetNumMips() const
    {
        return NumMips;
    }

    FORCEINLINE uint32 GetFlags() const
    {
        return Flags;
    }

    FORCEINLINE const SClearValue& GetOptimalClearValue() const
    {
        return OptimalClearValue;
    }

    FORCEINLINE bool IsUAV() const
    {
        return (Flags & TextureFlag_UAV) && !(Flags & TextureFlag_NoDefaultUAV);
    }

    FORCEINLINE bool IsSRV() const
    {
        return (Flags & TextureFlag_SRV) && !(Flags & TextureFlag_NoDefaultSRV);
    }

    FORCEINLINE bool IsRTV() const
    {
        return (Flags & TextureFlag_RTV) && !(Flags & TextureFlag_NoDefaultRTV);
    }

    FORCEINLINE bool IsDSV() const
    {
        return (Flags & TextureFlag_SRV) && !(Flags & TextureFlag_NoDefaultSRV);
    }

private:
    EFormat Format;
    uint32  NumMips;
    uint32  Flags;
    SClearValue OptimalClearValue;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class CRHITexture2D : public CRHITexture
{
public:

    CRHITexture2D( EFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InNumMips, uint32 InNumSamples, uint32 InFlags, const SClearValue& InOptimizedClearValue )
        : CRHITexture( InFormat, InNumMips, InFlags, InOptimizedClearValue )
        , Width( InWidth )
        , Height( InHeight )
        , NumSamples( InNumSamples )
    {
    }

    /* Cast to Texture2D */
    virtual CRHITexture2D* AsTexture2D() override { return this; }

    // Returns a RenderTargetView if texture is created with TextureFlag_RTV
    virtual class CRHIRenderTargetView* GetRenderTargetView() const { return nullptr; }
    // Returns a DepthStencilView if texture is created with TextureFlag_DSV
    virtual class CRHIDepthStencilView* GetDepthStencilView() const { return nullptr; }
    // Returns a UnorderedAccessView if texture is created with TextureFlag_UAV
    virtual class CRHIUnorderedAccessView* GetUnorderedAccessView() const { return nullptr; }

    FORCEINLINE uint32 GetWidth() const
    {
        return Width;
    }

    FORCEINLINE uint32 GetHeight() const
    {
        return Height;
    }

    FORCEINLINE uint32 GetNumSamples() const
    {
        return NumSamples;
    }

    FORCEINLINE bool IsMultiSampled() const
    {
        return NumSamples > 1;
    }

protected:
    FORCEINLINE void SetSize( uint32 InWidth, uint32 InHeight )
    {
        Width = InWidth;
        Height = InHeight;
    }

private:
    uint32 Width;
    uint32 Height;
    uint32 NumSamples;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class CRHITexture2DArray : public CRHITexture2D
{
public:

    CRHITexture2DArray( EFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InNumMips, uint32 InNumSamples, uint32 InNumArraySlices, uint32 InFlags, const SClearValue& InOptimizedClearValue )
        : CRHITexture2D( InFormat, InWidth, InHeight, InNumMips, InNumSamples, InFlags, InOptimizedClearValue )
        , NumArraySlices( InNumArraySlices )
    {
    }

    /* Cast to Texture2D */
    virtual CRHITexture2D* AsTexture2D() override { return nullptr; }
    /* Cast to Texture2DArray */
    virtual CRHITexture2DArray* AsTexture2DArray() override { return this; }

    FORCEINLINE uint32 GetNumArraySlices() const
    {
        return NumArraySlices;
    }

private:
    uint32 NumArraySlices;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class CRHITextureCube : public CRHITexture
{
public:

    CRHITextureCube( EFormat InFormat, uint32 InSize, uint32 InNumMips, uint32 InFlags, const SClearValue& InOptimizedClearValue )
        : CRHITexture( InFormat, InNumMips, InFlags, InOptimizedClearValue )
        , Size( InSize )
    {
    }

    /* Cast to TextureCube */
    virtual CRHITextureCube* AsTextureCube() override { return this; }

    FORCEINLINE uint32 GetSize() const
    {
        return Size;
    }

private:
    uint32 Size;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class CRHITextureCubeArray : public CRHITextureCube
{
public:
    CRHITextureCubeArray( EFormat InFormat, uint32 InSize, uint32 InNumMips, uint32 InNumArraySlices, uint32 InFlags, const SClearValue& InOptimizedClearValue )
        : CRHITextureCube( InFormat, InSize, InNumMips, InFlags, InOptimizedClearValue )
        , NumArraySlices( InNumArraySlices )
    {
    }

    /* Cast to TextureCube */
    virtual CRHITextureCube* AsTextureCube() override { return nullptr; }
    /* Cast to TextureCubeArray */
    virtual CRHITextureCubeArray* AsTextureCubeArray() override { return this; }

    FORCEINLINE uint32 GetNumArraySlices() const
    {
        return NumArraySlices;
    }

private:
    uint32 NumArraySlices;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class CRHITexture3D : public CRHITexture
{
public:

    CRHITexture3D( EFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InDepth, uint32 InNumMips, uint32 InFlags, const SClearValue& InOptimizedClearValue )
        : CRHITexture( InFormat, InNumMips, InFlags, InOptimizedClearValue )
        , Width( InWidth )
        , Height( InHeight )
        , Depth( InDepth )
    {
    }

    /* Cast to Texture3D */
    virtual CRHITexture3D* AsTexture3D() override { return this; }

    FORCEINLINE uint32 GetWidth()  const
    {
        return Width;
    }

    FORCEINLINE uint32 GetHeight() const
    {
        return Height;
    }

    FORCEINLINE uint32 GetDepth()  const
    {
        return Depth;
    }

private:
    uint32 Width;
    uint32 Height;
    uint32 Depth;
};