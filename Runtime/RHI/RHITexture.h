#pragma once
#include "Core/Containers/String.h"
#include "RHI/RHIResource.h"

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

NODISCARD constexpr bool IsTextureCube(ETextureDimension Dimension)
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
    NODISCARD
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

    NODISCARD
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

    NODISCARD
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

    NODISCARD
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

    NODISCARD
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

    FRHITextureInfo() noexcept = default;

    FRHITextureInfo(
        ETextureDimension  InDimension,
        EFormat            InFormat,
        FIntVector3        InExtent,
        uint32             InNumArraySlices,
        uint32             InNumMipLevels,
        uint32             InNumSamples,
        ETextureUsageFlags InUsageFlags,
        const FClearValue& InClearValue = FClearValue()) noexcept
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

    NODISCARD bool IsTexture2D()        const { return (Dimension == ETextureDimension::Texture2D); }
    NODISCARD bool IsTexture2DArray()   const { return (Dimension == ETextureDimension::Texture2DArray); }
    NODISCARD bool IsTextureCube()      const { return (Dimension == ETextureDimension::TextureCube); }
    NODISCARD bool IsTextureCubeArray() const { return (Dimension == ETextureDimension::TextureCubeArray); }
    NODISCARD bool IsTexture3D()        const { return (Dimension == ETextureDimension::Texture3D); }

    NODISCARD bool IsShaderResource()  const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::ShaderResource); }
    NODISCARD bool IsUnorderedAccess() const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::UnorderedAccess); }
    NODISCARD bool IsRenderTarget()    const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::RenderTarget); }
    NODISCARD bool IsDepthStencil()    const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::DepthStencil); }
    NODISCARD bool IsPresentable()     const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::Presentable); }
    NODISCARD bool IsMultisampled()    const { return (NumSamples > 1); }

    bool operator==(const FRHITextureInfo& Other) const noexcept = default;

    ETextureDimension  Dimension      = ETextureDimension::None;
    EFormat            Format         = EFormat::Unknown;
    ETextureUsageFlags UsageFlags     = ETextureUsageFlags::None;
    uint32             NumArraySlices = 0;
    uint32             NumMipLevels   = 0;
    uint32             NumSamples     = 0;
    FIntVector3        Extent         = { };
    FClearValue        ClearValue     = { };
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
    
    // Returns the native handle for this resource
    virtual void* GetRHINativeHandle() const { return nullptr; }

    virtual FRHIShaderResourceView* GetShaderResourceView() const { return nullptr; }
    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const { return FRHIDescriptorHandle(); }

    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const { return nullptr; }
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
