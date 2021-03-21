#pragma once
#include "ResourceBase.h"

enum ETextureFlags
{
    TextureFlag_None          = 0,
    TextureFlag_RTV           = FLAG(1), // RenderTargetView
    TextureFlag_DSV           = FLAG(2), // DepthStencilView
    TextureFlag_UAV           = FLAG(3), // UnorderedAccessView
    TextureFlag_SRV           = FLAG(4), // ShaderResourceView
    TextureFlag_NoDefaultRTV  = FLAG(5), // Do not create default RenderTargetView
    TextureFlag_NoDefaultDSV  = FLAG(6), // Do not create default DepthStencilView
    TextureFlag_NoDefaultUAV  = FLAG(7), // Do not create default UnorderedAccessView
    TextureFlag_NoDefaultSRV  = FLAG(8), // Do not create default ShaderResourceView
    TextureFlags_RWTexture    = TextureFlag_UAV | TextureFlag_SRV,
    TextureFlags_RenderTarget = TextureFlag_RTV | TextureFlag_SRV,
    TextureFlags_ShadowMap    = TextureFlag_DSV | TextureFlag_SRV,
};

class Texture : public Resource
{
public:
    Texture(EFormat InFormat, uint32 InNumMips, uint32 InFlags, const ClearValue& InOptimalClearValue)
        : Resource()
        , Format(InFormat)
        , NumMips(InNumMips)
        , Flags(InFlags)
        , OptimalClearValue(InOptimalClearValue)
    {
    }

    virtual class Texture2D*        AsTexture2D()        { return nullptr; }
    virtual class Texture2DArray*   AsTexture2DArray()   { return nullptr; }
    virtual class TextureCube*      AsTextureCube()      { return nullptr; }
    virtual class TextureCubeArray* AsTextureCubeArray() { return nullptr; }
    virtual class Texture3D*        AsTexture3D()        { return nullptr; }

    // Returns a ShaderResourceView of the full resource if texture is created with TextureFlag_SRV
    virtual class ShaderResourceView* GetShaderResourceView() const { return nullptr; }

    EFormat GetFormat() const { return Format; }

    uint32 GetNumMips() const { return NumMips; }

    uint32 GetFlags() const { return Flags; }

    const ClearValue& GetOptimalClearValue() const { return OptimalClearValue; }

    // Checks weather a default shaderrsourceview is created by the renderlayer
    bool IsUAV() const { return (Flags & TextureFlag_UAV) && !(Flags & TextureFlag_NoDefaultUAV); }
    bool IsSRV() const { return (Flags & TextureFlag_SRV) && !(Flags & TextureFlag_NoDefaultSRV); }
    bool IsRTV() const { return (Flags & TextureFlag_RTV) && !(Flags & TextureFlag_NoDefaultRTV); }
    bool IsDSV() const { return (Flags & TextureFlag_SRV) && !(Flags & TextureFlag_NoDefaultSRV); }

private:
    EFormat Format;
    uint32  NumMips;
    uint32  Flags;
    ClearValue OptimalClearValue;
};

class Texture2D : public Texture
{
public:
    Texture2D(EFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InNumMips, uint32 InNumSamples, uint32 InFlags, const ClearValue& InOptimizedClearValue)
        : Texture(InFormat, InNumMips, InFlags, InOptimizedClearValue)
        , Width(InWidth)
        , Height(InHeight)
        , NumSamples(InNumSamples)
    {
    }

    virtual Texture2D* AsTexture2D() override { return this; }

    // Returns a RenderTargetView if texture is created with TextureFlag_RTV
    virtual class RenderTargetView* GetRenderTargetView() const { return nullptr; }

    // Returns a DepthStencilView if texture is created with TextureFlag_DSV
    virtual class DepthStencilView* GetDepthStencilView() const { return nullptr; }

    // Returns a UnorderedAccessView if texture is created with TextureFlag_UAV
    virtual class UnorderedAccessView* GetUnorderedAccessView() const { return nullptr; }

    uint32 GetWidth() const { return Width; }
    uint32 GetHeight() const { return Height; }

    uint32 GetNumSamples() const { return NumSamples; }

    bool IsMultiSampled() const { return NumSamples > 1; }

private:
    uint32 Width;
    uint32 Height;
    uint32 NumSamples;
};

class Texture2DArray : public Texture2D
{
public:
    Texture2DArray(
        EFormat InFormat, 
        uint32 InWidth, 
        uint32 InHeight, 
        uint32 InNumMips, 
        uint32 InNumSamples, 
        uint32 InNumArraySlices, 
        uint32 InFlags, 
        const ClearValue& InOptimizedClearValue)
        : Texture2D(InFormat, InWidth, InHeight, InNumMips, InNumSamples, InFlags, InOptimizedClearValue)
        , NumArraySlices(InNumArraySlices)
    {
    }

    virtual Texture2D*      AsTexture2D()      override { return nullptr; }
    virtual Texture2DArray* AsTexture2DArray() override { return this; }

    uint32 GetNumArraySlices() const { return NumArraySlices; }

private:
    uint32 NumArraySlices;
};

class TextureCube : public Texture
{
public:
    TextureCube(EFormat InFormat, uint32 InSize, uint32 InNumMips, uint32 InFlags, const ClearValue& InOptimizedClearValue)
        : Texture(InFormat, InNumMips, InFlags, InOptimizedClearValue)
        , Size(InSize)
    {
    }

    virtual TextureCube* AsTextureCube() override { return this; }

    uint32 GetSize() const { return Size; }

private:
    uint32 Size;
};

class TextureCubeArray : public TextureCube
{
public:
    TextureCubeArray(EFormat InFormat, uint32 InSize, uint32 InNumMips, uint32 InNumArraySlices, uint32 InFlags, const ClearValue& InOptimizedClearValue)
        : TextureCube(InFormat, InSize, InNumMips, InFlags, InOptimizedClearValue)
        , NumArraySlices(InNumArraySlices)
    {
    }

    virtual TextureCube*      AsTextureCube()      override { return nullptr; }
    virtual TextureCubeArray* AsTextureCubeArray() override { return this; }

    uint32 GetNumArraySlices() const { return NumArraySlices; }

private:
    uint32 NumArraySlices;
};

class Texture3D : public Texture
{
public:
    Texture3D(EFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InDepth, uint32 InNumMips, uint32 InFlags, const ClearValue& InOptimizedClearValue)
        : Texture(InFormat, InNumMips, InFlags, InOptimizedClearValue)
        , Width(InWidth)
        , Height(InHeight)
        , Depth(InDepth)
    {
    }

    virtual Texture3D* AsTexture3D() override { return this; }

    uint32 GetWidth()  const { return Width; }
    uint32 GetHeight() const { return Height; }
    uint32 GetDepth()  const { return Depth; }

private:
    uint32 Width;
    uint32 Height;
    uint32 Depth;
};