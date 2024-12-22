#pragma once
#include "RHIResource.h"
#include "Core/Containers/String.h"

enum class ETextureUsageFlags
{
    None            = 0,
    RenderTarget    = FLAG(1), // RenderTargetView
    DepthStencil    = FLAG(2), // DepthStencilView
    UnorderedAccess = FLAG(3), // UnorderedAccessView
    ShaderResource  = FLAG(4), // ShaderResourceView
    Presentable     = FLAG(5), // Indicates that the resource is a BackBuffer resource
};

ENUM_CLASS_OPERATORS(ETextureUsageFlags);

enum class ETextureDimension
{
    None             = 0,
    Texture2D        = 1,
    Texture2DArray   = 2,
    TextureCube      = 3,
    TextureCubeArray = 4,
    Texture3D        = 5,
};

constexpr bool IsTextureCube(ETextureDimension Dimension)
{
    return Dimension == ETextureDimension::TextureCube || Dimension == ETextureDimension::TextureCubeArray;
}

struct IRHITextureData
{
    virtual ~IRHITextureData() = default;

    virtual int64 GetMipRowPitch(uint32 MipLevel = 0) const = 0;

    virtual int64 GetMipSlicePitch(uint32 MipLevel = 0) const = 0;

    virtual void* GetMipData(uint32 MipLevel = 0) const = 0;
};

struct FRHITextureInfo
{
    static FRHITextureInfo CreateTexture2D(
        EFormat            InFormat,
        uint32             InWidth,
        uint32             InHeight,
        uint32             InNumMipLevels,
        uint32             InNumSamples,
        ETextureUsageFlags InUsageFlags,
        const FClearValue& InClearValue = FClearValue())
    {
        return FRHITextureInfo(ETextureDimension::Texture2D, InFormat, FIntVector3(InWidth, InHeight, 0), 1, InNumMipLevels, InNumSamples, InUsageFlags, InClearValue);
    }

    static FRHITextureInfo CreateTexture2DArray(
        EFormat            InFormat,
        uint32             InWidth,
        uint32             InHeight,
        uint32             InArraySlices,
        uint32             InNumMipLevels,
        uint32             InNumSamples,
        ETextureUsageFlags InUsageFlags,
        const FClearValue& InClearValue = FClearValue())
    {
        return FRHITextureInfo(ETextureDimension::Texture2DArray, InFormat, FIntVector3(InWidth, InHeight, 0), InArraySlices, InNumMipLevels, InNumSamples, InUsageFlags, InClearValue);
    }

    static FRHITextureInfo CreateTextureCube(
        EFormat            InFormat,
        uint32             InExtent,
        uint32             InNumMipLevels,
        uint32             InNumSamples,
        ETextureUsageFlags InUsageFlags,
        const FClearValue& InClearValue = FClearValue())
    {
        return FRHITextureInfo(ETextureDimension::TextureCube, InFormat, FIntVector3(InExtent, InExtent, 0), 1, InNumMipLevels, InNumSamples, InUsageFlags, InClearValue);
    }

    static FRHITextureInfo CreateTextureCubeArray(
        EFormat            InFormat,
        uint32             InExtent,
        uint32             InArraySlices,
        uint32             InNumMipLevels,
        uint32             InNumSamples,
        ETextureUsageFlags InUsageFlags,
        const FClearValue& InClearValue = FClearValue())
    {
        return FRHITextureInfo(ETextureDimension::TextureCubeArray, InFormat, FIntVector3(InExtent, InExtent, 0), InArraySlices, InNumMipLevels, InNumSamples, InUsageFlags, InClearValue);
    }

    static FRHITextureInfo CreateTexture3D(
        EFormat            InFormat,
        uint32             InWidth,
        uint32             InHeight,
        uint32             InDepth,
        uint32             InNumMipLevels,
        uint32             InNumSamples,
        ETextureUsageFlags InUsageFlags,
        const FClearValue& InClearValue = FClearValue())
    {
        return FRHITextureInfo(ETextureDimension::Texture3D, InFormat, FIntVector3(InWidth, InHeight, InDepth), 1, InNumMipLevels, InNumSamples, InUsageFlags, InClearValue);
    }

    FRHITextureInfo()
        : Dimension(ETextureDimension::None)
        , Format(EFormat::Unknown)
        , UsageFlags(ETextureUsageFlags::None)
        , Extent()
        , NumArraySlices(0)
        , NumMipLevels(0)
        , NumSamples(0)
        , ClearValue()
    {
    }

    FRHITextureInfo(
        ETextureDimension  InDimension,
        EFormat            InFormat,
        FIntVector3        InExtent,
        uint32             InNumArraySlices,
        uint32             InNumMipLevels,
        uint32             InNumSamples,
        ETextureUsageFlags InUsageFlags,
        const FClearValue& InClearValue = FClearValue())
        : Dimension(InDimension)
        , Format(InFormat)
        , UsageFlags(InUsageFlags)
        , Extent(InExtent)
        , NumArraySlices(InNumArraySlices)
        , NumMipLevels(InNumMipLevels)
        , NumSamples(InNumSamples)
        , ClearValue(InClearValue)
    {
    }

    bool IsTexture2D()        const { return (Dimension == ETextureDimension::Texture2D); }
    bool IsTexture2DArray()   const { return (Dimension == ETextureDimension::Texture2DArray); }
    bool IsTextureCube()      const { return (Dimension == ETextureDimension::TextureCube); }
    bool IsTextureCubeArray() const { return (Dimension == ETextureDimension::TextureCubeArray); }
    bool IsTexture3D()        const { return (Dimension == ETextureDimension::Texture3D); }

    bool IsShaderResource()  const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::ShaderResource); }
    bool IsUnorderedAccess() const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::UnorderedAccess); }
    bool IsRenderTarget()    const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::RenderTarget); }
    bool IsDepthStencil()    const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::DepthStencil); }
    bool IsPresentable()     const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::Presentable); }
    bool IsMultisampled()    const { return (NumSamples > 1); }

    bool operator==(const FRHITextureInfo& Other) const
    {
        return Dimension      == Other.Dimension
            && Format         == Other.Format
            && UsageFlags     == Other.UsageFlags
            && Extent         == Other.Extent
            && NumArraySlices == Other.NumArraySlices
            && NumMipLevels   == Other.NumMipLevels
            && NumSamples     == Other.NumSamples
            && ClearValue     == Other.ClearValue;
    }

    bool operator!=(const FRHITextureInfo& Other) const
    {
        return !(*this == Other);
    }

    ETextureDimension  Dimension;
    EFormat            Format;
    ETextureUsageFlags UsageFlags;
    FIntVector3        Extent;
    uint32             NumArraySlices;
    uint32             NumMipLevels;
    uint32             NumSamples;
    FClearValue        ClearValue;
};

class FRHITexture : public FRHIResource
{
protected:
    explicit FRHITexture(const FRHITextureInfo& InTextureInfo)
        : FRHIResource()
        , Info(InTextureInfo)
    {
    }

public:
    
    virtual void* GetRHIBaseTexture()        { return nullptr; }
    virtual void* GetRHIBaseResource() const { return nullptr; }

    virtual FRHIShaderResourceView*  GetShaderResourceView()  const { return nullptr; }
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const { return nullptr; }
    
    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const { return FRHIDescriptorHandle(); }
    virtual FRHIDescriptorHandle GetBindlessUAVHandle() const { return FRHIDescriptorHandle(); }

    virtual void SetDebugName(const FString&) { }
    virtual FString GetDebugName() const { return ""; }

public:
    
    ETextureDimension GetDimension() const
    {
        return Info.Dimension;
    }
    
    EFormat GetFormat() const
    {
        return Info.Format;
    }
    
    ETextureUsageFlags GetFlags() const
    {
        return Info.UsageFlags;
    }
    
    const FIntVector3& GetExtent() const
    {
        return Info.Extent;
    }

    uint32 GetWidth() const
    {
        return Info.Extent.X;
    }
    
    uint32 GetHeight() const
    {
        return Info.Extent.Y;
    }
    
    uint32 GetDepth() const
    {
        return Info.Extent.Z;
    }
    
    uint32 GetNumArraySlices() const
    {
        return Info.NumArraySlices;
    }
    
    uint32 GetNumMipLevels() const
    {
        return Info.NumMipLevels;
    }
    
    uint32 GetNumSamples() const
    {
        return Info.NumSamples;
    }
    
    const FRHITextureInfo& GetInfo() const
    {
        return Info;
    }

protected:
    FRHITextureInfo Info;
};
