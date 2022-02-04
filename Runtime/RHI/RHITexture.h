#pragma once
#include "RHIResourceBase.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHITextureFlags

enum ERHITextureFlags
{
    TextureFlag_None = 0,
    TextureFlag_RTV = FLAG(1), // RenderTargetView
    TextureFlag_DSV = FLAG(2), // DepthStencilView
    TextureFlag_UAV = FLAG(3), // UnorderedAccessView
    TextureFlag_SRV = FLAG(4), // ShaderResourceView
    TextureFlag_NoDefaultRTV = FLAG(5), // Do not create default RenderTargetView
    TextureFlag_NoDefaultDSV = FLAG(6), // Do not create default DepthStencilView
    TextureFlag_NoDefaultUAV = FLAG(7), // Do not create default UnorderedAccessView
    TextureFlag_NoDefaultSRV = FLAG(8), // Do not create default ShaderResourceView
    TextureFlags_RWTexture = TextureFlag_UAV | TextureFlag_SRV,
    TextureFlags_RenderTarget = TextureFlag_RTV | TextureFlag_SRV,
    TextureFlags_ShadowMap = TextureFlag_DSV | TextureFlag_SRV,
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITexture

class CRHITexture : public CRHIResource
{
public:

    /**
     * Constructor taking parameters for creating a texture
     * 
     * @param InFormat: Format of the texture
     * @param InNumMips: Number of MipLevels of the texture
     * @param InFlags: Flags of the texture
     * @InOptimalClearValue: Optimized clear value for the texture
     */
    CRHITexture(EFormat InFormat, uint32 InNumMips, uint32 InFlags, const SClearValue& InOptimalClearValue)
        : CRHIResource()
        , Format(InFormat)
        , NumMips(InNumMips)
        , Flags(InFlags)
        , OptimalClearValue(InOptimalClearValue)
    {
    }

    /**
     * Cast resource to texture 
     * 
     * @return: Returns a pointer to a Texture
     */
    virtual CRHITexture* AsTexture() { return this; }

    /**
     * Cast to Texture2D 
     * 
     * @return: Returns a pointer to a Texture2D if the texture if of correct type
     */
    virtual class CRHITexture2D* AsTexture2D() { return nullptr; }

    /**
     * Cast to Texture2DArray
     *
     * @return: Returns a pointer to a Texture2DArray if the texture if of correct type
     */
    virtual class CRHITexture2DArray* AsTexture2DArray() { return nullptr; }
    
    /**
     * Cast to TextureCube
     *
     * @return: Returns a pointer to a TextureCube if the texture if of correct type
     */
    virtual class CRHITextureCube* AsTextureCube() { return nullptr; }

    /**
     * Cast to TextureCubeArray
     *
     * @return: Returns a pointer to a TextureCubeArray if the texture if of correct type
     */
    virtual class CRHITextureCubeArray* AsTextureCubeArray() { return nullptr; }

    /**
     * Cast to Texture3D
     *
     * @return: Returns a pointer to a Texture3D if the texture if of correct type
     */
    virtual class CRHITexture3D* AsTexture3D() { return nullptr; }

    /**
     * Returns a ShaderResourceView of the full resource if texture is created with TextureFlag_SRV
     * 
     * @return: Returns a pointer to a ShaderResourceView
     */
    virtual class CRHIShaderResourceView* GetShaderResourceView() const { return nullptr; }

    /**
     * Retrieve the format of the texture
     * 
     * @return: Returns the format of the texture
     */
    FORCEINLINE EFormat GetFormat() const
    {
        return Format;
    }

    /**
     * Retrieve the number of MipLevels of the texture
     *
     * @return: Returns the number of MipLevels of the texture
     */
    FORCEINLINE uint32 GetNumMips() const
    {
        return NumMips;
    }

    /**
     * Retrieve the flags of the texture
     *
     * @return: Returns the flags of the texture
     */
    FORCEINLINE uint32 GetFlags() const
    {
        return Flags;
    }

    /**
     * Retrieve the optimized clear-value of the texture
     *
     * @return: Returns the optimized clear-value of the texture
     */
    FORCEINLINE const SClearValue& GetOptimalClearValue() const
    {
        return OptimalClearValue;
    }

    /**
     * Check if the texture can be used as a UnorderedAccessView
     * 
     * @return: Returns true if the texture was created with the UnorderedAccessView-flag
     */
    FORCEINLINE bool IsUAV() const
    {
        return (Flags & TextureFlag_UAV) && !(Flags & TextureFlag_NoDefaultUAV);
    }

    /**
     * Check if the texture can be used as a ShaderResourceView
     *
     * @return: Returns true if the texture was created with the ShaderResourceView-flag
     */
    FORCEINLINE bool IsSRV() const
    {
        return (Flags & TextureFlag_SRV) && !(Flags & TextureFlag_NoDefaultSRV);
    }

    /**
     * Check if the texture can be used as a RenderTargetView
     *
     * @return: Returns true if the texture was created with the RenderTargetView-flag
     */
    FORCEINLINE bool IsRTV() const
    {
        return (Flags & TextureFlag_RTV) && !(Flags & TextureFlag_NoDefaultRTV);
    }

    /**
     * Check if the texture can be used as a DepthStencilView
     *
     * @return: Returns true if the texture was created with the DepthStencilView-flag
     */
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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITexture2D

class CRHITexture2D : public CRHITexture
{
public:

    /**
     * Constructor taking parameters for creating a texture
     *
     * @param InFormat: Format of the texture
     * @param InWidth: Width of the texture
     * @param InHeight: Height of the texture
     * @param InNumMips: Number of MipLevels of the texture
     * @param InNumSamples: Number of samples of the texture
     * @param InFlags: Flags of the texture
     * @InOptimalClearValue: Optimized clear value for the texture
     */
    CRHITexture2D(EFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InNumMips, uint32 InNumSamples, uint32 InFlags, const SClearValue& InOptimizedClearValue)
        : CRHITexture(InFormat, InNumMips, InFlags, InOptimizedClearValue)
        , Width(InWidth)
        , Height(InHeight)
        , NumSamples(InNumSamples)
    {
    }

    /**
     * Cast to Texture2D
     *
     * @return: Returns a pointer to a Texture2D if the texture if of correct type
     */
    virtual CRHITexture2D* AsTexture2D() override { return this; }

    /**
     * Returns a RenderTargetView of the full resource if texture is created with TextureFlag_SRV
     *
     * @return: Returns a pointer to a RenderTargetView
     */
    virtual class CRHIRenderTargetView* GetRenderTargetView() const { return nullptr; }

    /**
     * Returns a DepthStencilView of the full resource if texture is created with TextureFlag_SRV
     *
     * @return: Returns a pointer to a DepthStencilView
     */
    virtual class CRHIDepthStencilView* GetDepthStencilView() const { return nullptr; }

    /**
     * Returns a UnorderedAccessView of the full resource if texture is created with TextureFlag_SRV
     *
     * @return: Returns a pointer to a UnorderedAccessView
     */
    virtual class CRHIUnorderedAccessView* GetUnorderedAccessView() const { return nullptr; }

    /**
     * Retrieve the width of the texture
     * 
     * @return: Returns the width of the texture
     */
    FORCEINLINE uint32 GetWidth() const
    {
        return Width;
    }

    /**
     * Retrieve the height of the texture
     *
     * @return: Returns the height of the texture
     */
    FORCEINLINE uint32 GetHeight() const
    {
        return Height;
    }

    /**
     * Retrieve the number of samples of the texture
     *
     * @return: Returns the number of samples of the texture
     */
    FORCEINLINE uint32 GetNumSamples() const
    {
        return NumSamples;
    }

    /**
     * Check if the texture is multi-sampled
     * 
     * @return: Returns true if the number of samples is more than one
     */
    FORCEINLINE bool IsMultiSampled() const
    {
        return NumSamples > 1;
    }

protected:
    FORCEINLINE void SetSize(uint32 InWidth, uint32 InHeight)
    {
        Width = InWidth;
        Height = InHeight;
    }

private:
    uint32 Width;
    uint32 Height;
    uint32 NumSamples;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITexture2DArray

class CRHITexture2DArray : public CRHITexture2D
{
public:

    /**
     * Constructor taking parameters for creating a texture
     *
     * @param InFormat: Format of the texture
     * @param InWidth: Width of the texture
     * @param InHeight: Height of the texture
     * @param InNumMips: Number of MipLevels of the texture
     * @param InNumSamples: Number of samples of the texture
     * @param InNumArraySlices: Number of array-slices of the texture
     * @param InFlags: Flags of the texture
     * @InOptimalClearValue: Optimized clear value for the texture
     */
    CRHITexture2DArray(EFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InNumMips, uint32 InNumSamples, uint32 InNumArraySlices, uint32 InFlags, const SClearValue& InOptimizedClearValue)
        : CRHITexture2D(InFormat, InWidth, InHeight, InNumMips, InNumSamples, InFlags, InOptimizedClearValue)
        , NumArraySlices(InNumArraySlices)
    {
    }

    /**
     * Cast to Texture2D
     *
     * @return: Returns a pointer to a Texture2D if the texture if of correct type
     */
    virtual CRHITexture2D* AsTexture2D() override { return nullptr; }
    
    /**
     * Cast to Texture2DArray
     *
     * @return: Returns a pointer to a Texture2DArray if the texture if of correct type
     */
    virtual CRHITexture2DArray* AsTexture2DArray() override { return this; }

    /**
     * Retrieve the number of array-slices of the texture
     *
     * @return: Returns the number of array-slices of the texture
     */
    FORCEINLINE uint32 GetNumArraySlices() const
    {
        return NumArraySlices;
    }

private:
    uint32 NumArraySlices;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITextureCube

class CRHITextureCube : public CRHITexture
{
public:

    /**
     * Constructor taking parameters for creating a texture
     *
     * @param InFormat: Format of the texture
     * @param InSize: Width and height of the texture faces
     * @param InNumMips: Number of MipLevels of the texture
     * @param InFlags: Flags of the texture
     * @InOptimalClearValue: Optimized clear value for the texture
     */
    CRHITextureCube(EFormat InFormat, uint32 InSize, uint32 InNumMips, uint32 InFlags, const SClearValue& InOptimizedClearValue)
        : CRHITexture(InFormat, InNumMips, InFlags, InOptimizedClearValue)
        , Size(InSize)
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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITextureCubeArray

class CRHITextureCubeArray : public CRHITextureCube
{
public:

    /**
     * Constructor taking parameters for creating a texture
     *
     * @param InFormat: Format of the texture
     * @param InSize: Width and height of the texture faces
     * @param InNumMips: Number of MipLevels of the texture
     * @param InNumArraySlices: Number of array-slices of the texture
     * @param InFlags: Flags of the texture
     * @InOptimalClearValue: Optimized clear value for the texture
     */
    CRHITextureCubeArray(EFormat InFormat, uint32 InSize, uint32 InNumMips, uint32 InNumArraySlices, uint32 InFlags, const SClearValue& InOptimizedClearValue)
        : CRHITextureCube(InFormat, InSize, InNumMips, InFlags, InOptimizedClearValue)
        , NumArraySlices(InNumArraySlices)
    {
    }

    /**
     * Cast to TextureCube
     *
     * @return: Returns a pointer to a TextureCube if the texture if of correct type
     */
    virtual CRHITextureCube* AsTextureCube() override { return nullptr; }
    
    /**
     * Cast to TextureCubeArray
     *
     * @return: Returns a pointer to a TextureCubeArray if the texture if of correct type
     */
    virtual CRHITextureCubeArray* AsTextureCubeArray() override { return this; }

    /**
     * Retrieve the number of array-slices of the texture
     *
     * @return: Returns the number of array-slices of the texture
     */
    FORCEINLINE uint32 GetNumArraySlices() const
    {
        return NumArraySlices;
    }

private:
    uint32 NumArraySlices;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

class CRHITexture3D : public CRHITexture
{
public:

    /**
     * Constructor taking parameters for creating a texture
     *
     * @param InFormat: Format of the texture
     * @param InWidth: Width of the texture
     * @param InHeight: Height of the texture
     * @param InDepth: Depth of the texture
     * @param InNumMips: Number of MipLevels of the texture
     * @param InFlags: Flags of the texture
     * @InOptimalClearValue: Optimized clear value for the texture
     */
    CRHITexture3D(EFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InDepth, uint32 InNumMips, uint32 InFlags, const SClearValue& InOptimizedClearValue)
        : CRHITexture(InFormat, InNumMips, InFlags, InOptimizedClearValue)
        , Width(InWidth)
        , Height(InHeight)
        , Depth(InDepth)
    {
    }

    /**
     * Cast to Texture3D
     *
     * @return: Returns a pointer to a Texture3D if the texture if of correct type
     */
    virtual CRHITexture3D* AsTexture3D() override { return this; }
    
    /**
     * Retrieve the width of the texture
     *
     * @return: Returns the width of the texture
     */
    FORCEINLINE uint32 GetWidth() const
    {
        return Width;
    }

    /**
     * Retrieve the height of the texture
     *
     * @return: Returns the height of the texture
     */
    FORCEINLINE uint32 GetHeight() const
    {
        return Height;
    }

    /**
     * Retrieve the depth of the texture
     *
     * @return: Returns the depth of the texture
     */
    FORCEINLINE uint32 GetDepth()  const
    {
        return Depth;
    }

private:
    uint32 Width;
    uint32 Height;
    uint32 Depth;
};