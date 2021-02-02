#pragma once
#include "ResourceBase.h"

enum ETextureFlags
{
    TextureFlag_None          = 0,
    TextureFlag_RTV           = FLAG(1), // RenderTargetView
    TextureFlag_DSV           = FLAG(2), // DepthStencilView
    TextureFlag_UAV           = FLAG(3), // UnorderedAccessView
    TextureFlag_SRV           = FLAG(4), // ShaderResourceView
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

    ~Texture() = default;

    virtual class Texture2D* AsTexture2D() { return nullptr; }
    virtual class Texture2DArray* AsTexture2DArray() { return nullptr; }
    virtual class TextureCube* AsTextureCube() { return nullptr; }
    virtual class TextureCubeArray* AsTextureCubeArray() { return nullptr; }
    virtual class Texture3D* AsTexture3D() { return nullptr; }

    EFormat GetFormat() const { return Format; }

    UInt32 GetNumMiplevels() const { return NumMips; }

    UInt32 GetFlags() const { return Flags; }

    const ClearValue& GetOptimalClearValue() const { return OptimalClearValue; }

    Bool IsUAV() const { return (Flags & TextureFlag_UAV); }
    Bool IsSRV() const { return (Flags & TextureFlag_SRV); }
    Bool IsRTV() const { return (Flags & TextureFlag_RTV); }
    Bool IsDSV() const { return (Flags & TextureFlag_SRV); }

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

    ~Texture2D() = default;

    virtual Texture2D* AsTexture2D() override { return this; }

    UInt32 GetWidth() const { return Width; }
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

    ~Texture2DArray() = default;

    virtual Texture2D* AsTexture2D() override { return nullptr; }
    virtual Texture2DArray* AsTexture2DArray() override { return this; }

    UInt32 GetNumArraySlices() const { return NumArraySlices; }

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

    ~TextureCube() = default;

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

    ~TextureCubeArray() = default;

    virtual TextureCube* AsTextureCube() override { return nullptr; }
    virtual TextureCubeArray* AsTextureCubeArray() override { return this; }

    UInt32 GetNumArraySlices() const { return NumArraySlices; }

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

    ~Texture3D() = default;

    virtual Texture3D* AsTexture3D() override { return this; }

    UInt32 GetWidth() const { return Width; }
    UInt32 GetHeight() const { return Height; }
    UInt32 GetDepth() const { return Depth; }

private:
    UInt32 Width;
    UInt32 Height;
    UInt32 Depth;
};