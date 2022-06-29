#pragma once
#include "RHITypes.h"
#include "RHIResourceBase.h"

#include "Core/Math/IntVector3.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Templates/EnumUtilities.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedefs

typedef TSharedRef<class FRHITexture>        FRHITextureRef;
typedef TSharedRef<class FRHITexture2D>      FRHITexture2DRef;
typedef TSharedRef<class FRHITexture2DArray> FRHITexture2DArrayRef;
typedef TSharedRef<class FRHITextureCube>    FRHITextureCubeRef;
typedef TSharedRef<class FRHITexture3D>      FRHITexture3DRef;

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
// FRHITextureDataInitializer

class FRHITextureDataInitializer
{
public:

    FRHITextureDataInitializer()
        : TextureData(nullptr)
        , Size(0)
    { }

    explicit FRHITextureDataInitializer(const void* InBufferData, EFormat Format, uint32 Width, uint32 Height)
        : TextureData(InBufferData)
        , Size(Width * Height * GetByteStrideFromFormat(Format))
    { }

    bool operator==(const FRHITextureDataInitializer& RHS) const
    {
        return (TextureData == RHS.TextureData) && (Size == RHS.Size);
    }

    bool operator!=(const FRHITextureDataInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    const void* TextureData;
    uint32      Size;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHITextureInitializer

class FRHITextureInitializer
{
public:

    FRHITextureInitializer()
        : ClearValue()
        , Format(EFormat::Unknown)
        , UsageFlags(ETextureUsageFlags::None)
        , InitialAccess(EResourceAccess::Common)
        , InitialData(nullptr)
        , NumMips(1)
    { }

    FRHITextureInitializer( EFormat InFormat
                          , ETextureUsageFlags InUsageFlags
                          , EResourceAccess InInitialAccess
                          , uint32 InNumMips
                          , FRHITextureDataInitializer* InInitialData = nullptr
                          , const FTextureClearValue& InClearValue = FTextureClearValue())
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

    bool operator==(const FRHITextureInitializer& RHS) const
    {
        return (ClearValue    == RHS.ClearValue)
            && (Format        == RHS.Format)
            && (UsageFlags    == RHS.UsageFlags)
            && (InitialAccess == RHS.InitialAccess)
            && (InitialData   == RHS.InitialData)
            && (NumMips       == RHS.NumMips);
    }

    bool operator!=(const FRHITextureInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    FTextureClearValue          ClearValue;

    EFormat                     Format;

    ETextureUsageFlags          UsageFlags;
    EResourceAccess             InitialAccess;

    FRHITextureDataInitializer* InitialData;

    uint8                       NumMips;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHITexture2DInitializer

class FRHITexture2DInitializer : public FRHITextureInitializer
{
public:

    FRHITexture2DInitializer()
        : FRHITextureInitializer()
        , Width(1)
        , Height(1)
        , NumMips(1)
        , NumSamples(1)
    { }

    FRHITexture2DInitializer( EFormat InFormat
                            , uint32 InWidth
                            , uint32 InHeight
                            , uint32 InNumMips
                            , uint32 InNumSamples
                            , ETextureUsageFlags InUsageFlags
                            , EResourceAccess InInitialAccess
                            , FRHITextureDataInitializer* InInitialData = nullptr
                            , const FTextureClearValue& InClearValue = FTextureClearValue())
        : FRHITextureInitializer(InFormat, InUsageFlags, InInitialAccess, InNumMips, InInitialData, InClearValue)
        , Width(uint16(InWidth))
        , Height(uint16(InHeight))
        , NumMips(uint8(InNumMips))
        , NumSamples(uint8(InNumSamples))
    { }

    bool operator==(const FRHITexture2DInitializer& RHS) const
    {
        return FRHITextureInitializer::operator==(RHS)
            && (Width      == RHS.Width)
            && (Height     == RHS.Height)
            && (NumMips    == RHS.NumMips)
            && (NumSamples == RHS.NumSamples);
    }

    bool operator!=(const FRHITexture2DInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    uint16 Width;
    uint16 Height;

    uint8  NumMips;
    uint8  NumSamples;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHITexture2DArrayInitializer

class FRHITexture2DArrayInitializer : public FRHITexture2DInitializer
{
public:

    FRHITexture2DArrayInitializer()
        : FRHITexture2DInitializer()
        , ArraySize(1)
    { }

    FRHITexture2DArrayInitializer( EFormat InFormat
                                 , uint32 InWidth
                                 , uint32 InHeight
                                 , uint32 InArraySize
                                 , uint32 InNumMips
                                 , uint32 InNumSamples
                                 , ETextureUsageFlags InUsageFlags
                                 , EResourceAccess InInitialAccess
                                 , FRHITextureDataInitializer* InInitialData = nullptr
                                 , const FTextureClearValue& InClearValue = FTextureClearValue())
        : FRHITexture2DInitializer(InFormat, InWidth, InHeight, InNumMips, InNumSamples, InUsageFlags, InInitialAccess, InInitialData, InClearValue)
        , ArraySize(uint16(InArraySize))
    { }

    bool operator==(const FRHITexture2DArrayInitializer& RHS) const
    {
        return FRHITexture2DInitializer::operator==(RHS) && (ArraySize == RHS.ArraySize);
    }

    bool operator!=(const FRHITexture2DArrayInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    uint16 ArraySize;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHITextureCubeInitializer

class FRHITextureCubeInitializer : public FRHITextureInitializer
{
public:

    FRHITextureCubeInitializer()
        : FRHITextureInitializer()
	    , NumSamples(1)
	    , Extent(1)
    { }

    FRHITextureCubeInitializer( EFormat InFormat
                              , uint32 InExtent
                              , uint32 InNumMips
                              , uint32 InNumSamples
                              , ETextureUsageFlags InUsageFlags
                              , EResourceAccess InInitialAccess
                              , FRHITextureDataInitializer* InInitialData = nullptr
                              , const FTextureClearValue& InClearValue = FTextureClearValue())
        : FRHITextureInitializer(InFormat, InUsageFlags, InInitialAccess, InNumMips, InInitialData, InClearValue)
	    , NumSamples(uint8(InNumSamples))
	    , Extent(uint16(InExtent))
    { }

    bool operator==(const FRHITextureCubeInitializer& RHS) const
    {
        return FRHITextureInitializer::operator==(RHS)
            && (Extent     == RHS.Extent)
            && (NumSamples == RHS.NumSamples);
    }

    bool operator!=(const FRHITextureCubeInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    uint8  NumSamples;
    uint16 Extent;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHITextureCubeArrayInitializer

class FRHITextureCubeArrayInitializer : public FRHITextureCubeInitializer
{
public:

    FRHITextureCubeArrayInitializer()
        : FRHITextureCubeInitializer()
        , ArraySize(1)
    { }

    FRHITextureCubeArrayInitializer( EFormat InFormat
                                   , uint32 InExtent
                                   , uint32 InArraySize
                                   , uint32 InNumMips
                                   , uint32 InNumSamples
                                   , ETextureUsageFlags InUsageFlags
                                   , EResourceAccess InInitialAccess
                                   , FRHITextureDataInitializer* InInitialData = nullptr
                                   , const FTextureClearValue& InClearValue = FTextureClearValue())
        : FRHITextureCubeInitializer(InFormat, InExtent, InNumMips, InNumSamples, InUsageFlags, InInitialAccess, InInitialData, InClearValue)
        , ArraySize(uint16(InArraySize))
    { }

    bool operator==(const FRHITextureCubeArrayInitializer& RHS) const
    {
        return FRHITextureCubeInitializer::operator==(RHS) && (ArraySize == RHS.ArraySize);
    }

    bool operator!=(const FRHITextureCubeArrayInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    uint16 ArraySize;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHITexture3DInitializer

class FRHITexture3DInitializer : public FRHITextureInitializer
{
public:

    FRHITexture3DInitializer()
        : FRHITextureInitializer()
        , Width(1)
        , Height(1)
        , Depth(1)
    { }

    FRHITexture3DInitializer( EFormat InFormat
                            , uint16 InWidth
                            , uint16 InHeight
                            , uint16 InDepth
                            , uint8 InNumMips
                            , ETextureUsageFlags InUsageFlags
                            , EResourceAccess InInitialAccess
                            , FRHITextureDataInitializer* InInitialData = nullptr
                            , const FTextureClearValue& InClearValue = FTextureClearValue())
        : FRHITextureInitializer(InFormat, InUsageFlags, InInitialAccess, InNumMips, InInitialData, InClearValue)
        , Width(InWidth)
        , Height(InHeight)
        , Depth(InDepth)
    { }

    bool operator==(const FRHITexture3DInitializer& RHS) const
    {
        return FRHITextureInitializer::operator==(RHS)
            && (Width  == RHS.Width)
            && (Height == RHS.Height)
            && (Depth  == RHS.Depth);
    }

    bool operator!=(const FRHITexture3DInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    uint16 Width;
    uint16 Height;
    uint16 Depth;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHITexture

class FRHITexture : public FRHIResource
{
protected:

    explicit FRHITexture(const FRHITextureInitializer& Initializer)
        : FRHIResource()
        , Format(Initializer.Format)
        , NumMips(Initializer.NumMips)
        , UsageFlags(Initializer.UsageFlags)
        , ClearValue(Initializer.ClearValue)
    { }

public:

    virtual class FRHITexture2D* GetTexture2D() { return nullptr; }

    virtual class FRHITexture2DArray* GetTexture2DArray() { return nullptr; }
    
    virtual class FRHITextureCube* GetTextureCube() { return nullptr; }

    virtual class FRHITextureCubeArray* GetTextureCubeArray() { return nullptr; }

    virtual class FRHITexture3D* GetTexture3D() { return nullptr; }

    virtual void* GetRHIBaseTexture() { return nullptr; }

    virtual void* GetRHIBaseResource() const { return nullptr; }

    virtual class FRHIShaderResourceView* GetShaderResourceView() const { return nullptr; }

    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const { return FRHIDescriptorHandle(); }

    virtual uint32 GetWidth() const { return 1; }

    virtual uint32 GetHeight() const { return 1; }

    virtual uint32 GetDepth() const { return 1; }

    virtual FIntVector3 GetExtent() const { return FIntVector3(1, 1, 1); }

    virtual uint32 GetArraySize() const { return 1; }

    virtual uint32 GetNumSamples() const { return 1; }

    virtual void SetName(const String& InName) { }

    virtual String GetName() const { return ""; }

public:
    
    bool IsMultiSampled() const { return (GetNumSamples() > 1); }

    ETextureUsageFlags GetFlags() const { return UsageFlags; }

    EFormat GetFormat() const { return Format; }

    uint32 GetNumMips() const { return NumMips; }

    const FTextureClearValue& GetClearValue() const { return ClearValue; }

protected:
    EFormat            Format;
    uint8              NumMips;
    ETextureUsageFlags UsageFlags;
    FTextureClearValue ClearValue;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHITexture2D

class FRHITexture2D : public FRHITexture
{
protected:

    explicit FRHITexture2D(const FRHITexture2DInitializer& Initializer)
        : FRHITexture(Initializer)
        , NumSamples(Initializer.NumSamples)
        , Width(Initializer.Width)
        , Height(Initializer.Height)
    {
        Check(Width  != 0);
        Check(Height != 0);
    }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHITexture Interface

    virtual FRHITexture2D* GetTexture2D() override { return this; }

    virtual uint32 GetWidth() const override final { return Width; }
    
    virtual uint32 GetHeight() const override final { return Height; }
    
    virtual FIntVector3 GetExtent() const override { return FIntVector3(Width, Height, 1); }

    virtual uint32 GetNumSamples() const override final { return NumSamples; }

public:

    virtual class FRHIUnorderedAccessView* GetUnorderedAccessView() const { return nullptr; }

protected:
    uint8  NumSamples;
    uint16 Width;
    uint16 Height;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHITexture2DArray

class FRHITexture2DArray : public FRHITexture2D
{
protected:

    explicit FRHITexture2DArray(const FRHITexture2DArrayInitializer& Initializer)
        : FRHITexture2D(Initializer)
        , ArraySize(Initializer.ArraySize)
    {
        Check(ArraySize != 0);
    }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHITexture Interface

    virtual FRHITexture2D* GetTexture2D() override final { return nullptr; }
    
    virtual FRHITexture2DArray* GetTexture2DArray() override final { return this; }

    virtual FIntVector3 GetExtent() const override final { return FIntVector3(GetWidth(), GetDepth(), ArraySize); }

    virtual uint32 GetArraySize() const override final { return ArraySize; }

protected:
    uint16 ArraySize;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHITextureCube

class FRHITextureCube : public FRHITexture
{
protected:

    explicit FRHITextureCube(const FRHITextureCubeInitializer& Initializer)
        : FRHITexture(Initializer)
        , NumSamples(Initializer.NumSamples)
	    , Extent(Initializer.Extent)
    {
        Check(Extent != 0);
    }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHITexture Interface

    virtual FRHITextureCube* GetTextureCube() override { return this; }

    virtual uint32 GetWidth()  const override final { return Extent; }
    
    virtual uint32 GetHeight() const override final { return Extent; }

    virtual FIntVector3 GetExtent() const override { return FIntVector3(Extent, Extent, 1); }
    
    virtual uint32 GetNumSamples() const override final { return NumSamples; }

protected:
    uint8  NumSamples;
    uint16 Extent;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHITextureCubeArray

class FRHITextureCubeArray : public FRHITextureCube
{
protected:

    explicit FRHITextureCubeArray(const FRHITextureCubeArrayInitializer& Initializer)
        : FRHITextureCube(Initializer)
        , ArraySize(Initializer.ArraySize)
    {
        Check(ArraySize != 0);
    }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHITexture Interface

    virtual FRHITextureCube* GetTextureCube() override final { return nullptr; }
    
    virtual FRHITextureCubeArray* GetTextureCubeArray() override final { return this; }

    virtual FIntVector3 GetExtent() const override final { return FIntVector3(GetWidth(), GetHeight(), ArraySize); }

    virtual uint32 GetArraySize() const override final { return ArraySize; }

protected:
    uint16 ArraySize;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHITexture3D

class FRHITexture3D : public FRHITexture
{
protected:

    explicit FRHITexture3D(const FRHITexture3DInitializer& Initializer)
        : FRHITexture(Initializer)
        , Width(Initializer.Width)
        , Height(Initializer.Height)
        , Depth(Initializer.Depth)
    {
        Check(Width  != 0);
        Check(Height != 0);
        Check(Depth  != 0);
    }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHITexture Interface

    virtual FRHITexture3D* GetTexture3D() override { return this; }

    virtual uint32 GetWidth()  const override final { return Width; }
    
    virtual uint32 GetHeight() const override final { return Height; }

    virtual uint32 GetDepth()  const override final { return Depth; }

    virtual FIntVector3 GetExtent() const override final { return FIntVector3(Width, Height, Depth); }

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
