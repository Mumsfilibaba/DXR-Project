#pragma once
#include "RHIResourceBase.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedefs

typedef TSharedRef<class CRHITexture>          CRHITextureRef;
typedef TSharedRef<class CRHITexture2D>        CRHITexture2DRef;
typedef TSharedRef<class CRHITexture2DArray>   CRHITexture2DArrayRef;
typedef TSharedRef<class CRHITextureCube>      CRHITextureCubeRef;
typedef TSharedRef<class CRHITextureCubeArray> CRHITextureCubeArrayRef;
typedef TSharedRef<class CRHITexture3D>        CRHITexture3DRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHITextureFlags

enum ERHITextureFlags : uint16
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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHITextureDimension

enum class ERHITextureDimension : uint8
{
    Unknown          = 0,
    Texture2D        = 1,
    Texture2DArray   = 2,
    TextureCube      = 3,
    TextureCubeArray = 4,
    Texture3D        = 5
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITextureDesc

class CRHITextureDesc
{
public:
    static CRHITextureDesc CreateTexture2D(ERHIFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InNumMips, uint32 InNumSamples, uint32 InFlags)
    {
        return CRHITextureDesc(ERHITextureDimension::Texture2D, InFormat, InWidth, InHeight, 1, InNumMips, InNumSamples, InFlags);
    }

    static CRHITextureDesc CreateTexture2DArray(ERHIFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InArraySize, uint32 InNumMips, uint32 InNumSamples, uint32 InFlags)
    {
        return CRHITextureDesc(ERHITextureDimension::Texture2DArray, InFormat, InWidth, InHeight, InArraySize, InNumMips, InNumSamples, InFlags);
    }

    static CRHITextureDesc CreateTextureCube(ERHIFormat InFormat, uint32 Size, uint32 InNumMips, uint32 InNumSamples, uint32 InFlags)
    {
        return CRHITextureDesc(ERHITextureDimension::TextureCube, InFormat, Size, Size, 1, InNumMips, InNumSamples, InFlags);
    }

    static CRHITextureDesc CreateTextureCubeArray(ERHIFormat InFormat, uint32 Size, uint32 InArraySize, uint32 InNumMips, uint32 InNumSamples, uint32 InFlags)
    {
        return CRHITextureDesc(ERHITextureDimension::TextureCubeArray, InFormat, Size, Size, InArraySize, InNumMips, InNumSamples, InFlags);
    }

    static CRHITextureDesc CreateTexture3D(ERHIFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InDepth, uint32 InNumMips, uint32 InNumSamples, uint32 InFlags)
    {
        return CRHITextureDesc(ERHITextureDimension::Texture3D, InFormat, InWidth, InHeight, InDepth, InNumMips, InNumSamples, InFlags);
    }

    CRHITextureDesc()  = default;
    ~CRHITextureDesc() = default;

    CRHITextureDesc(ERHITextureDimension InDimension, ERHIFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InDepthOrArraySize, uint32 InNumMips, uint32 InNumSamples, uint32 InFlags)
        : Dimension(InDimension)
        , Format(InFormat)
        , Width(InWidth)
        , Height(InHeight)
        , DepthOrArraySize(InDepthOrArraySize)
        , NumMips(InNumMips)
        , NumSamples(InNumSamples)
        , Flags(InFlags)
    { }

    bool IsUAV() const
    {
        return (Flags & TextureFlag_UAV);
    }

    bool IsSRV() const
    {
        return (Flags & TextureFlag_SRV);
    }

    bool IsRTV() const
    {
        return (Flags & TextureFlag_RTV);
    }

    bool IsDSV() const
    {
        return (Flags & TextureFlag_DSV);
    }

    bool IsRWTexture() const
    {
        return (Flags & TextureFlags_RWTexture);
    }

    bool operator==(const CRHITextureDesc& RHS) const
    {
        return (Format == RHS.Format) && (Width == RHS.Width) && (Height == RHS.Height) && (DepthOrArraySize == RHS.DepthOrArraySize) && 
            (NumMips == RHS.NumMips) && (NumSamples == RHS.NumSamples) && (Flags == RHS.Flags) && (ClearValue == RHS.ClearValue);
    }

    bool operator!=(const CRHITextureDesc& RHS) const
    {
        return !(*this == RHS);
    }

    ERHITextureDimension Dimension        = ERHITextureDimension::Unknown;
    ERHIFormat           Format           = ERHIFormat::Unknown;
    uint16               Width            = 0;
    uint16               Height           = 0;
    uint16               DepthOrArraySize = 0;
    uint8                NumMips          = 0;
    uint8                NumSamples       = 0;
    uint16               Flags            = 0;
    CRHIClearValue       ClearValue       = CRHIClearValue();
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITexture

class CRHITexture : public CRHIResource
{
public:

    CRHITexture(const CRHITextureDesc& InTextureDesc)
        : CRHIResource(ERHIResourceType::Texture)
        , Dimension(InTextureDesc.Dimension)
        , Format(InTextureDesc.Format)
        , Flags(InTextureDesc.Flags)
        , NumMips(InTextureDesc.NumMips)
        , ClearValue(InTextureDesc.ClearValue)
    { }

    virtual CRHITexture* AsTexture() { return this; }

    virtual class CRHITexture2D*        AsTexture2D()        { return nullptr; }
    virtual class CRHITexture2DArray*   AsTexture2DArray()   { return nullptr; }
    virtual class CRHITextureCube*      AsTextureCube()      { return nullptr; }
    virtual class CRHITextureCubeArray* AsTextureCubeArray() { return nullptr; }
    virtual class CRHITexture3D*        AsTexture3D()        { return nullptr; }

    inline bool IsUAV() const { return (Flags & TextureFlag_UAV); }
    inline bool IsSRV() const { return (Flags & TextureFlag_SRV); }
    inline bool IsRTV() const { return (Flags & TextureFlag_RTV); }
    inline bool IsDSV() const { return (Flags & TextureFlag_DSV); }

    virtual uint16      GetArraySize() const { return 1; }
    virtual uint16      GetSizeX()     const { return 1; }
    virtual uint16      GetSizeY()     const { return 1; }
    virtual uint16      GetSizeZ()     const { return 1; }
    virtual CIntVector3 GetSizeXYZ()   const { return CIntVector3(1, 1, 1); }

    inline const CRHIClearValue& GetClearValue() const { return ClearValue; }

    inline ERHIFormat GetFormat()  const { return Format; }

    inline uint16 GetFlags() const { return Flags; }

    inline uint8 GetNumMips() const { return NumMips; }

    inline ERHITextureDimension GetDimension() const { return Dimension; }

protected:
    ERHITextureDimension Dimension;
    ERHIFormat           Format;
    
    uint16               Flags;
    uint8                NumMips;

    CRHIClearValue       ClearValue;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITexture2D

class CRHITexture2D : public CRHITexture
{
public:

    CRHITexture2D(const CRHITextureDesc& InTextureDesc)
        : CRHITexture(InTextureDesc)
        , Width(InTextureDesc.Width)
        , Height(InTextureDesc.Height)
        , NumSamples(InTextureDesc.NumSamples)
    { }

    virtual CRHITexture2D* AsTexture2D() override { return this; }

    virtual uint16      GetSizeX()   const override final { return Width; }
    virtual uint16      GetSizeY()   const override final { return Height; }
    virtual CIntVector3 GetSizeXYZ() const override final { return CIntVector3(Width, Height, 1); }

    inline bool IsMultiSampled() const { return (NumSamples > 1u); }

    inline uint16 GetWidth()  const { return Width; }
    inline uint16 GetHeight() const { return Height; }

    inline uint8 GetNumSamples() const { return NumSamples; }

protected:
    uint16 Width;
    uint16 Height;
    uint8  NumSamples;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITexture2DArray

class CRHITexture2DArray : public CRHITexture2D
{
public:

    CRHITexture2DArray(const CRHITextureDesc& InTextureDesc)
        : CRHITexture2D(InTextureDesc)
        , ArraySize(InTextureDesc.DepthOrArraySize)
    { }

    virtual CRHITexture2D*      AsTexture2D()      override { return nullptr; }
    virtual CRHITexture2DArray* AsTexture2DArray() override { return this; }

    virtual uint16 GetArraySize() const override final { return ArraySize; }

protected:
    uint16 ArraySize;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITextureCube

class CRHITextureCube : public CRHITexture
{
public:

    CRHITextureCube(const CRHITextureDesc& InTextureDesc)
        : CRHITexture(InTextureDesc)
        , Size(InTextureDesc.Width)
    { }

    virtual CRHITextureCube* AsTextureCube() override { return this; }

    virtual uint16      GetSizeX()   const override final { return Size; }
    virtual uint16      GetSizeY()   const override final { return Size; }
    virtual CIntVector3 GetSizeXYZ() const override final { return CIntVector3(Size, Size, 1); }

    inline uint16 GetSize() const { return Size; }

protected:
    uint16 Size;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITextureCubeArray

class CRHITextureCubeArray : public CRHITextureCube
{
public:

    CRHITextureCubeArray(const CRHITextureDesc& InTextureDesc)
        : CRHITextureCube(InTextureDesc)
        , ArraySize(InTextureDesc.DepthOrArraySize)
    { }

    virtual CRHITextureCube*      AsTextureCube()      override { return nullptr; }
    virtual CRHITextureCubeArray* AsTextureCubeArray() override { return this;    }

    virtual uint16 GetArraySize() const override final { return ArraySize; }

protected:
    uint16 ArraySize;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITexture3D

class CRHITexture3D : public CRHITexture
{
public:

    CRHITexture3D(const CRHITextureDesc& InTextureDesc)
        : CRHITexture(InTextureDesc)
        , Width(InTextureDesc.Width)
        , Height(InTextureDesc.Height)
        , Depth(InTextureDesc.DepthOrArraySize)
    { }

    virtual CRHITexture3D* AsTexture3D() override { return this; }
    
    virtual uint16      GetSizeX()   const override final { return Width; }
    virtual uint16      GetSizeY()   const override final { return Height; }
    virtual uint16      GetSizeZ()   const override final { return Depth; }
    virtual CIntVector3 GetSizeXYZ() const override final { return CIntVector3(Width, Height, Depth); }

    inline uint16 GetWidth()  const { return Width; }
    inline uint16 GetHeight() const { return Height; }
    inline uint16 GetDepth()  const { return Depth; }

protected:
    uint16 Width;
    uint16 Height;
    uint16 Depth;
};
