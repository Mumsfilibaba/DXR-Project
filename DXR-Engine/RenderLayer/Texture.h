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
    Texture(EFormat InFormat, UInt32 InNumMips, UInt32 InFlags, const ClearValue& InOptimalClearValue)
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

    virtual UInt32 GetArraySize() const { return 1; }

    EFormat GetFormat() const { return Format; }

    UInt32 GetNumMips() const { return NumMips; }

    UInt32 GetFlags() const { return Flags; }

    const ClearValue& GetOptimalClearValue() const { return OptimalClearValue; }

    // Checks weather a default shaderrsourceview is created by the renderlayer
    Bool IsUAV() const { return (Flags & TextureFlag_UAV) && !(Flags & TextureFlag_NoDefaultUAV); }
    Bool IsSRV() const { return (Flags & TextureFlag_SRV) && !(Flags & TextureFlag_NoDefaultSRV); }
    Bool IsRTV() const { return (Flags & TextureFlag_RTV) && !(Flags & TextureFlag_NoDefaultRTV); }
    Bool IsDSV() const { return (Flags & TextureFlag_SRV) && !(Flags & TextureFlag_NoDefaultSRV); }

private:
    EFormat Format;
    UInt32  NumMips;
    UInt32  Flags;
    ClearValue OptimalClearValue;
};

class Texture2D : public Texture
{
public:
    Texture2D(EFormat InFormat, UInt32 InWidth, UInt32 InHeight, UInt32 InNumMips, UInt32 InNumSamples, UInt32 InFlags, const ClearValue& InOptimizedClearValue)
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

    UInt32 GetWidth()  const { return Width; }
    UInt32 GetHeight() const { return Height; }

    UInt32 GetNumSamples() const { return NumSamples; }

    Bool IsMultiSampled() const { return NumSamples > 1; }

private:
    UInt32 Width;
    UInt32 Height;
    UInt32 NumSamples;
};

class Texture2DArray : public Texture2D
{
public:
    Texture2DArray(
        EFormat InFormat, 
        UInt32 InWidth, 
        UInt32 InHeight, 
        UInt32 InNumMips, 
        UInt32 InNumSamples, 
        UInt32 InNumArraySlices, 
        UInt32 InFlags, 
        const ClearValue& InOptimizedClearValue)
        : Texture2D(InFormat, InWidth, InHeight, InNumMips, InNumSamples, InFlags, InOptimizedClearValue)
        , NumArraySlices(InNumArraySlices)
    {
    }

    virtual Texture2D*      AsTexture2D()      override { return nullptr; }
    virtual Texture2DArray* AsTexture2DArray() override { return this; }

    virtual UInt32 GetArraySize() const override { return NumArraySlices; }

private:
    UInt32 NumArraySlices;
};

class TextureCube : public Texture
{
public:
    TextureCube(EFormat InFormat, UInt32 InSize, UInt32 InNumMips, UInt32 InFlags, const ClearValue& InOptimizedClearValue)
        : Texture(InFormat, InNumMips, InFlags, InOptimizedClearValue)
        , Size(InSize)
    {
    }

    virtual TextureCube* AsTextureCube() override { return this; }

    UInt32 GetSize() const { return Size; }

private:
    UInt32 Size;
};

class TextureCubeArray : public TextureCube
{
public:
    TextureCubeArray(EFormat InFormat, UInt32 InSize, UInt32 InNumMips, UInt32 InNumArraySlices, UInt32 InFlags, const ClearValue& InOptimizedClearValue)
        : TextureCube(InFormat, InSize, InNumMips, InFlags, InOptimizedClearValue)
        , NumArraySlices(InNumArraySlices)
    {
    }

    virtual TextureCube*      AsTextureCube()      override { return nullptr; }
    virtual TextureCubeArray* AsTextureCubeArray() override { return this; }

    virtual UInt32 GetArraySize() const override { return NumArraySlices; }

private:
    UInt32 NumArraySlices;
};

class Texture3D : public Texture
{
public:
    Texture3D(EFormat InFormat, UInt32 InWidth, UInt32 InHeight, UInt32 InDepth, UInt32 InNumMips, UInt32 InFlags, const ClearValue& InOptimizedClearValue)
        : Texture(InFormat, InNumMips, InFlags, InOptimizedClearValue)
        , Width(InWidth)
        , Height(InHeight)
        , Depth(InDepth)
    {
    }

    virtual Texture3D* AsTexture3D() override { return this; }

    UInt32 GetWidth()  const { return Width; }
    UInt32 GetHeight() const { return Height; }
    UInt32 GetDepth()  const { return Depth; }

private:
    UInt32 Width;
    UInt32 Height;
    UInt32 Depth;
};