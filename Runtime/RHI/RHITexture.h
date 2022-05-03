#pragma once
#include "RHITypes.h"
#include "RHIResourceBase.h"

#include "Core/Templates/EnumUtilities.h"
#include "Core/Containers/SharedRef.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedefs

typedef TSharedRef<class CRHITexture>        RHITextureRef;
typedef TSharedRef<class CRHITexture2D>      RHITexture2DRef;
typedef TSharedRef<class CRHITexture2DArray> RHITexture2DArrayRef;
typedef TSharedRef<class CRHITextureCube>    RHITextureCubeRef;
typedef TSharedRef<class CRHITexture3D>      RHITexture3DRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ETextureUsageFlags

enum class ETextureUsageFlags
{
    None         = 0,
    AllowRTV     = FLAG(1), // RenderTargetView
    AllowDSV     = FLAG(2), // DepthStencilView
    AllowUAV     = FLAG(3), // UnorderedAccessView
    AllowSRV     = FLAG(4), // ShaderResourceView
    NoDefaultRTV = FLAG(5), // Do not create default RenderTargetView
    NoDefaultDSV = FLAG(6), // Do not create default DepthStencilView
    NoDefaultUAV = FLAG(7), // Do not create default UnorderedAccessView
    NoDefaultSRV = FLAG(8), // Do not create default ShaderResourceView
    
    RWTexture    = AllowUAV | AllowSRV,
    RenderTarget = AllowRTV | AllowSRV,
    ShadowMap    = AllowDSV | AllowSRV,
};

ENUM_CLASS_OPERATORS(ETextureUsageFlags);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITextureDataInitializer

class CRHITextureDataInitializer
{
public:

    CRHITextureDataInitializer()
        : TextureData(nullptr)
        , Size(0)
    { }

    explicit CRHITextureDataInitializer(const void* InBufferData, EFormat Format, uint32 Width, uint32 Height)
        : TextureData(InBufferData)
        , Size(Width * Height * GetByteStrideFromFormat(Format))
    { }

    bool operator==(const CRHITextureDataInitializer& RHS) const
    {
        return (TextureData == RHS.TextureData) && (Size == RHS.Size);
    }

    bool operator!=(const CRHITextureDataInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    const void* TextureData;
    uint32      Size;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITextureInitializer

class CRHITextureInitializer
{
public:

    CRHITextureInitializer()
        : ClearValue()
        , Format(EFormat::Unknown)
        , UsageFlags(ETextureUsageFlags::None)
        , InitialAccess(EResourceAccess::Common)
        , InitialData(nullptr)
        , NumMips(1)
    { }

    CRHITextureInitializer( EFormat InFormat
                          , ETextureUsageFlags InUsageFlags
                          , EResourceAccess InInitialAccess
                          , uint32 InNumMips
                          , CRHITextureDataInitializer* InInitialData = nullptr
                          , const CTextureClearValue& InClearValue = CTextureClearValue())
        : ClearValue(InClearValue)
        , Format(InFormat)
        , UsageFlags(InUsageFlags)
        , InitialAccess(InInitialAccess)
        , InitialData(InInitialData)
        , NumMips(uint8(InNumMips))
    { }

    bool AllowSRV() const { return ((UsageFlags & ETextureUsageFlags::AllowSRV) != ETextureUsageFlags::None); }

    bool AllowDefaultSRV() const { return AllowSRV() && ((UsageFlags & ETextureUsageFlags::NoDefaultSRV) == ETextureUsageFlags::None); }

    bool AllowUAV() const { return ((UsageFlags & ETextureUsageFlags::AllowUAV) != ETextureUsageFlags::None); }

    bool AllowDefaultUAV() const { return AllowUAV() && ((UsageFlags & ETextureUsageFlags::NoDefaultUAV) == ETextureUsageFlags::None); }

    bool AllowRTV() const { return ((UsageFlags & ETextureUsageFlags::AllowRTV) != ETextureUsageFlags::None); }

    bool AllowDefaultRTV() const { return AllowRTV() && ((UsageFlags & ETextureUsageFlags::NoDefaultRTV) == ETextureUsageFlags::None); }

    bool AllowDSV() const { return ((UsageFlags & ETextureUsageFlags::AllowDSV) != ETextureUsageFlags::None); }

    bool AllowDefaultDSV() const { return AllowDSV() && ((UsageFlags & ETextureUsageFlags::NoDefaultDSV) == ETextureUsageFlags::None); }

    bool operator==(const CRHITextureInitializer& RHS) const
    {
        return (ClearValue    == RHS.ClearValue)
            && (Format        == RHS.Format)
            && (UsageFlags    == RHS.UsageFlags)
            && (InitialAccess == RHS.InitialAccess)
            && (InitialData   == RHS.InitialData)
            && (NumMips       == RHS.NumMips);
    }

    bool operator!=(const CRHITextureInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    CTextureClearValue          ClearValue;

    EFormat                     Format;

    ETextureUsageFlags          UsageFlags;
    EResourceAccess             InitialAccess;

    CRHITextureDataInitializer* InitialData;

    uint8                       NumMips;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITexture2DInitializer

class CRHITexture2DInitializer : public CRHITextureInitializer
{
public:

    CRHITexture2DInitializer()
        : CRHITextureInitializer()
        , Width(1)
        , Height(1)
        , NumMips(1)
        , NumSamples(1)
    { }

    CRHITexture2DInitializer( EFormat InFormat
                            , uint32 InWidth
                            , uint32 InHeight
                            , uint32 InNumMips
                            , uint32 InNumSamples
                            , ETextureUsageFlags InUsageFlags
                            , EResourceAccess InInitialAccess
                            , CRHITextureDataInitializer* InInitialData = nullptr
                            , const CTextureClearValue& InClearValue = CTextureClearValue())
        : CRHITextureInitializer(InFormat, InUsageFlags, InInitialAccess, InNumMips, InInitialData, InClearValue)
        , Width(uint16(InWidth))
        , Height(uint16(InHeight))
        , NumMips(uint8(InNumMips))
        , NumSamples(uint8(InNumSamples))
    { }

    bool operator==(const CRHITexture2DInitializer& RHS) const
    {
        return CRHITextureInitializer::operator==(RHS)
            && (Width      == RHS.Width)
            && (Height     == RHS.Height)
            && (NumMips    == RHS.NumMips)
            && (NumSamples == RHS.NumSamples);
    }

    bool operator!=(const CRHITexture2DInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    uint16 Width;
    uint16 Height;

    uint8  NumMips;
    uint8  NumSamples;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITexture2DArrayInitializer

class CRHITexture2DArrayInitializer : public CRHITexture2DInitializer
{
public:

    CRHITexture2DArrayInitializer()
        : CRHITexture2DInitializer()
        , ArraySize(1)
    { }

    CRHITexture2DArrayInitializer( EFormat InFormat
                                 , uint32 InWidth
                                 , uint32 InHeight
                                 , uint32 InArraySize
                                 , uint32 InNumMips
                                 , uint32 InNumSamples
                                 , ETextureUsageFlags InUsageFlags
                                 , EResourceAccess InInitialAccess
                                 , CRHITextureDataInitializer* InInitialData = nullptr
                                 , const CTextureClearValue& InClearValue = CTextureClearValue())
        : CRHITexture2DInitializer(InFormat, InWidth, InHeight, InNumMips, InNumSamples, InUsageFlags, InInitialAccess, InInitialData, InClearValue)
        , ArraySize(uint16(InArraySize))
    { }

    bool operator==(const CRHITexture2DArrayInitializer& RHS) const
    {
        return CRHITexture2DInitializer::operator==(RHS) && (ArraySize == RHS.ArraySize);
    }

    bool operator!=(const CRHITexture2DArrayInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    uint16 ArraySize;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITextureCubeInitializer

class CRHITextureCubeInitializer : public CRHITextureInitializer
{
public:

    CRHITextureCubeInitializer()
        : CRHITextureInitializer()
        , Extent(1)
        , NumSamples(1)
    { }

    CRHITextureCubeInitializer( EFormat InFormat
                              , uint32 InExtent
                              , uint32 InNumMips
                              , uint32 InNumSamples
                              , ETextureUsageFlags InUsageFlags
                              , EResourceAccess InInitialAccess
                              , CRHITextureDataInitializer* InInitialData = nullptr
                              , const CTextureClearValue& InClearValue = CTextureClearValue())
        : CRHITextureInitializer(InFormat, InUsageFlags, InInitialAccess, InNumMips, InInitialData, InClearValue)
        , Extent(uint16(InExtent))
        , NumSamples(uint8(InNumSamples))
    { }

    bool operator==(const CRHITextureCubeInitializer& RHS) const
    {
        return CRHITextureInitializer::operator==(RHS)
            && (Extent     == RHS.Extent)
            && (NumSamples == RHS.NumSamples);
    }

    bool operator!=(const CRHITextureCubeInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    uint8  NumSamples;
    uint16 Extent;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITextureCubeArrayInitializer

class CRHITextureCubeArrayInitializer : public CRHITextureCubeInitializer
{
public:

    CRHITextureCubeArrayInitializer()
        : CRHITextureCubeInitializer()
        , ArraySize(1)
    { }

    CRHITextureCubeArrayInitializer( EFormat InFormat
                                   , uint32 InExtent
                                   , uint32 InArraySize
                                   , uint32 InNumMips
                                   , uint32 InNumSamples
                                   , ETextureUsageFlags InUsageFlags
                                   , EResourceAccess InInitialAccess
                                   , CRHITextureDataInitializer* InInitialData = nullptr
                                   , const CTextureClearValue& InClearValue = CTextureClearValue())
        : CRHITextureCubeInitializer(InFormat, InExtent, InNumMips, InNumSamples, InUsageFlags, InInitialAccess, InInitialData, InClearValue)
        , ArraySize(uint16(InArraySize))
    { }

    bool operator==(const CRHITextureCubeArrayInitializer& RHS) const
    {
        return CRHITextureCubeInitializer::operator==(RHS) && (ArraySize == RHS.ArraySize);
    }

    bool operator!=(const CRHITextureCubeArrayInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    uint16 ArraySize;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITexture3DInitializer

class CRHITexture3DInitializer : public CRHITextureInitializer
{
public:

    CRHITexture3DInitializer()
        : CRHITextureInitializer()
        , Width(1)
        , Height(1)
        , Depth(1)
    { }

    CRHITexture3DInitializer( EFormat InFormat
                            , uint16 InWidth
                            , uint16 InHeight
                            , uint16 InDepth
                            , uint8 InNumMips
                            , ETextureUsageFlags InUsageFlags
                            , EResourceAccess InInitialAccess
                            , CRHITextureDataInitializer* InInitialData = nullptr
                            , const CTextureClearValue& InClearValue = CTextureClearValue())
        : CRHITextureInitializer(InFormat, InUsageFlags, InInitialAccess, InNumMips, InInitialData, InClearValue)
        , Width(InWidth)
        , Height(InHeight)
        , Depth(InDepth)
    { }

    bool operator==(const CRHITexture3DInitializer& RHS) const
    {
        return CRHITextureInitializer::operator==(RHS)
            && (Width  == RHS.Width)
            && (Height == RHS.Height)
            && (Depth  == RHS.Depth);
    }

    bool operator!=(const CRHITexture3DInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    uint16 Width;
    uint16 Height;
    uint16 Depth;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITexture

class CRHITexture : public CRHIResource
{
protected:

    explicit CRHITexture(const CRHITextureInitializer& Initializer)
        : CRHIResource()
        , Format(Initializer.Format)
        , NumMips(Initializer.NumMips)
        , UsageFlags(Initializer.UsageFlags)
        , ClearValue(Initializer.ClearValue)
    { }

public:

    virtual class CRHITexture2D* GetTexture2D() { return nullptr; }

    virtual class CRHITexture2DArray* GetTexture2DArray() { return nullptr; }
    
    virtual class CRHITextureCube* GetTextureCube() { return nullptr; }

    virtual class CRHITextureCubeArray* GetTextureCubeArray() { return nullptr; }

    virtual class CRHITexture3D* GetTexture3D() { return nullptr; }

    virtual void* GetRHIBaseResource() const { return nullptr; }

    virtual void* GetRHIBaseTexture() { return nullptr; }

    virtual class CRHIShaderResourceView* GetDefaultShaderResourceView() const { return nullptr; }

    virtual CRHIDescriptorHandle GetDefaultBindlessSRVHandle() const { return CRHIDescriptorHandle(); }

    virtual uint32 GetWidth() const { return 1; }

    virtual uint32 GetHeight() const { return 1; }

    virtual uint32 GetDepth() const { return 1; }

    virtual CIntVector3 GetExtent() const { return CIntVector3(1, 1, 1); }

    virtual uint32 GetArraySize() const { return 1; }

    virtual uint32 GetNumSamples() const { return 1; }

    virtual void SetName(const String& InName) { }

    virtual String GetName() const { return ""; }

    bool IsMultiSampled() const { return (GetNumSamples() > 1); }

    ETextureUsageFlags GetFlags() const { return UsageFlags; }

    EFormat GetFormat() const { return Format; }

    uint32 GetNumMips() const { return NumMips; }

    const CTextureClearValue& GetClearValue() const { return ClearValue; }

protected:
    EFormat            Format;
    uint8              NumMips;
    ETextureUsageFlags UsageFlags;
    CTextureClearValue ClearValue;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITexture2D

class CRHITexture2D : public CRHITexture
{
protected:

    explicit CRHITexture2D(const CRHITexture2DInitializer& Initializer)
        : CRHITexture(Initializer)
        , NumSamples(Initializer.NumSamples)
        , Width(Initializer.Width)
        , Height(Initializer.Height)
    { }

public:

    virtual class CRHIRenderTargetView* GetRenderTargetView() const { return nullptr; }

    virtual class CRHIDepthStencilView* GetDepthStencilView() const { return nullptr; }

    virtual class CRHIUnorderedAccessView* GetUnorderedAccessView() const { return nullptr; }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITexture Interface

    virtual CRHITexture2D* GetTexture2D() override { return this; }

    virtual uint32 GetWidth() const override final { return Width; }
    
    virtual uint32 GetHeight() const override final { return Height; }
    
    virtual CIntVector3 GetExtent() const override { return CIntVector3(Width, Height, 1); }

    virtual uint32 GetNumSamples() const override final { return NumSamples; }

protected:
    uint8  NumSamples;
    uint16 Width;
    uint16 Height;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITexture2DArray

class CRHITexture2DArray : public CRHITexture2D
{
protected:

    explicit CRHITexture2DArray(const CRHITexture2DArrayInitializer& Initializer)
        : CRHITexture2D(Initializer)
        , ArraySize(Initializer.ArraySize)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITexture Interface

    virtual CRHITexture2D* GetTexture2D() override final { return nullptr; }
    
    virtual CRHITexture2DArray* GetTexture2DArray() override final { return this; }

    virtual CIntVector3 GetExtent() const override final { return CIntVector3(GetWidth(), GetDepth(), ArraySize); }

    virtual uint32 GetArraySize() const override final { return ArraySize; }

protected:
    uint16 ArraySize;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITextureCube

class CRHITextureCube : public CRHITexture
{
protected:

    explicit CRHITextureCube(const CRHITextureCubeInitializer& Initializer)
        : CRHITexture(Initializer)
        , Extent(Initializer.Extent)
        , NumSamples(Initializer.NumSamples)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITexture Interface

    virtual CRHITextureCube* GetTextureCube() override { return this; }

    virtual uint32 GetWidth()  const override final { return Extent; }
    
    virtual uint32 GetHeight() const override final { return Extent; }

    virtual CIntVector3 GetExtent() const override { return CIntVector3(Extent, Extent, 1); }
    
    virtual uint32 GetNumSamples() const override final { return NumSamples; }

protected:
    uint8  NumSamples;
    uint16 Extent;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITextureCubeArray

class CRHITextureCubeArray : public CRHITextureCube
{
protected:

    explicit CRHITextureCubeArray(const CRHITextureCubeArrayInitializer& Initializer)
        : CRHITextureCube(Initializer)
        , ArraySize(Initializer.ArraySize)
    { }

public:

    uint32 GetNumCubes() const { return ArraySize; }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITexture Interface

    virtual CRHITextureCube* GetTextureCube() override final { return nullptr; }
    
    virtual CRHITextureCubeArray* GetTextureCubeArray() override final { return this; }

    virtual CIntVector3 GetExtent() const override final { return CIntVector3(GetWidth(), GetHeight(), ArraySize); }

    virtual uint32 GetArraySize() const override final { return ArraySize; }

protected:
    uint16 ArraySize;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

class CRHITexture3D : public CRHITexture
{
protected:

    explicit CRHITexture3D(const CRHITexture3DInitializer& Initializer)
        : CRHITexture(Initializer)
        , Width(Initializer.Width)
        , Height(Initializer.Height)
        , Depth(Initializer.Depth)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHITexture Interface

    virtual CRHITexture3D* GetTexture3D() override { return this; }

    virtual uint32 GetWidth()  const override final { return Width; }
    
    virtual uint32 GetHeight() const override final { return Height; }

    virtual uint32 GetDepth()  const override final { return Depth; }

    virtual CIntVector3 GetExtent() const override final { return CIntVector3(Width, Height, Depth); }

protected:
    uint16 Width;
    uint16 Height;
    uint16 Depth;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif