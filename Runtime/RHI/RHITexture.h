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
// CRHITextureDesc

class CRHITextureDesc
{
public:
    static CRHITextureDesc CreateTexture2D(ERHIFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InNumMips, uint32 InNumSamples, uint32 InFlags)
    {
        return CRHITextureDesc(InFormat, InWidth, InHeight, 1, InNumMips, InNumSamples, InFlags);
    }

    static CRHITextureDesc CreateTexture2DArray(ERHIFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InArraySize, uint32 InNumMips, uint32 InNumSamples, uint32 InFlags)
    {
        return CRHITextureDesc(InFormat, InWidth, InHeight, InArraySize, InNumMips, InNumSamples, InFlags);
    }

    static CRHITextureDesc CreateTextureCube(ERHIFormat InFormat, uint32 Size, uint32 InNumMips, uint32 InNumSamples, uint32 InFlags)
    {
        return CRHITextureDesc(InFormat, Size, Size, 1, InNumMips, InNumSamples, InFlags);
    }

    static CRHITextureDesc CreateTextureCubeArray(ERHIFormat InFormat, uint32 Size, uint32 InArraySize, uint32 InNumMips, uint32 InNumSamples, uint32 InFlags)
    {
        return CRHITextureDesc(InFormat, Size, Size, InArraySize, InNumMips, InNumSamples, InFlags);
    }

    static CRHITextureDesc CreateTexture3D(ERHIFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InDepth, uint32 InNumMips, uint32 InNumSamples, uint32 InFlags)
    {
        return CRHITextureDesc(InFormat, InWidth, InHeight, InDepth, InNumMips, InNumSamples, InFlags);
    }

    CRHITextureDesc()  = default;
    ~CRHITextureDesc() = default;

    CRHITextureDesc(ERHIFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InDepthOrArraySize, uint32 InNumMips, uint32 InNumSamples, uint32 InFlags)
        : Format(InFormat)
        , Width(InWidth)
        , Height(InHeight)
        , DepthOrArraySize(InDepthOrArraySize)
        , NumMips(InNumMips)
        , NumSamples(InNumSamples)
        , Flags(InFlags)
    {
    }

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

    CRHIClearValue ClearValue       = CRHIClearValue();
    ERHIFormat     Format           = ERHIFormat::Unknown;
    uint16         Width            = 0;
    uint16         Height           = 0;
    uint16         DepthOrArraySize = 0;
    uint8          NumMips          = 0;
    uint8          NumSamples       = 0;
    uint16         Flags            = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITexture

class CRHITexture : public CRHIResource
{
public:

    CRHITexture(const CRHITextureDesc& InTextureDesc)
        : CRHIResource()
        , TextureDesc(InTextureDesc)
    {
    }

    virtual CRHITexture* AsTexture() { return this; }

    virtual class CRHITexture2D*        AsTexture2D()        { return nullptr; }
    virtual class CRHITexture2DArray*   AsTexture2DArray()   { return nullptr; }
    virtual class CRHITextureCube*      AsTextureCube()      { return nullptr; }
    virtual class CRHITextureCubeArray* AsTextureCubeArray() { return nullptr; }
    virtual class CRHITexture3D*        AsTexture3D()        { return nullptr; }

    inline bool IsUAV() const { return (TextureDesc.Flags & TextureFlag_UAV); }
    inline bool IsSRV() const { return (TextureDesc.Flags & TextureFlag_SRV); }
    inline bool IsRTV() const { return (TextureDesc.Flags & TextureFlag_RTV); }
    inline bool IsDSV() const { return (TextureDesc.Flags & TextureFlag_DSV); }

    inline ERHIFormat GetFormat()  const { return TextureDesc.Format; }

    inline uint32 GetFlags() const   { return TextureDesc.Flags; }
    inline uint32 GetNumMips() const { return TextureDesc.NumMips; }

    inline const CRHIClearValue& GetClearValue() const { return TextureDesc.ClearValue; }

protected:
    CRHITextureDesc TextureDesc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITexture2D

class CRHITexture2D : public CRHITexture
{
public:

    CRHITexture2D(const CRHITextureDesc& InTextureDesc)
        : CRHITexture(InTextureDesc)
    {
    }

    virtual CRHITexture2D* AsTexture2D() override { return this; }

    inline bool IsMultiSampled() const { return (TextureDesc.NumSamples > 1); }

    inline uint32 GetWidth()  const { return TextureDesc.Width; }
    inline uint32 GetHeight() const { return TextureDesc.Height; }

    inline uint32 GetNumSamples() const { return TextureDesc.NumSamples; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITexture2DArray

class CRHITexture2DArray : public CRHITexture2D
{
public:

    CRHITexture2DArray(const CRHITextureDesc& InTextureDesc)
        : CRHITexture2D(InTextureDesc)
    {
    }

    virtual CRHITexture2D*      AsTexture2D()      override { return nullptr; }
    virtual CRHITexture2DArray* AsTexture2DArray() override { return this; }

    inline uint32 GetNumArraySlices() const { return TextureDesc.DepthOrArraySize; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITextureCube

class CRHITextureCube : public CRHITexture
{
public:

    CRHITextureCube(const CRHITextureDesc& InTextureDesc)
        : CRHITexture(InTextureDesc)
    {
    }

    virtual CRHITextureCube* AsTextureCube() override { return this; }

    inline uint32 GetSize() const { return TextureDesc.Width; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITextureCubeArray

class CRHITextureCubeArray : public CRHITextureCube
{
public:

    CRHITextureCubeArray(const CRHITextureDesc& InTextureDesc)
        : CRHITextureCube(InTextureDesc)
    {
    }

    virtual CRHITextureCube*      AsTextureCube()      override { return nullptr; }
    virtual CRHITextureCubeArray* AsTextureCubeArray() override { return this;    }

    inline uint32 GetNumArraySlices() const { return TextureDesc.DepthOrArraySize; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITexture3D

class CRHITexture3D : public CRHITexture
{
public:

    CRHITexture3D(const CRHITextureDesc& InTextureDesc)
        : CRHITexture(InTextureDesc)
    {
    }

    virtual CRHITexture3D* AsTexture3D() override { return this; }
    
    inline uint32 GetWidth()  const { return TextureDesc.Width; }
    inline uint32 GetHeight() const { return TextureDesc.Height; }
    inline uint32 GetDepth()  const { return TextureDesc.DepthOrArraySize; }
};
