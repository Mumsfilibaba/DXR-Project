#pragma once
#include "Core/Containers/String.h"
#include "RHI/RHIResource.h"

enum class ETextureUsageFlags
{
    None = 0,

    RenderTarget           = FLAG(1), // RenderTargetView
    DepthStencil           = FLAG(2), // DepthStencilView
    UnorderedAccessTexture = FLAG(3), // UnorderedAccessView
    ShaderResourceTexture  = FLAG(4), // ShaderResourceView
    ShadingRateTexture     = FLAG(5), // Indicates that the texture is going to be used as a shading rate texture
    Presentable            = FLAG(6), // Indicates that the texture is a BackBuffer resource
    NoDefaultSRV           = FLAG(7), // Do not create a default ShaderResourceView at texture creation time
    NoDefaultUAV           = FLAG(8), // Do not create a default UnorderedAccessView at texture creation time
};

ENUM_CLASS_OPERATORS(ETextureUsageFlags);

enum class ETextureDimension
{
    None = 0,
    
    Texture1D        = 1,
    Texture1DArray   = 2,
    Texture2D        = 3,
    Texture2DArray   = 4,
    TextureCube      = 5,
    TextureCubeArray = 6,
    Texture3D        = 7,
};

NODISCARD constexpr const CHAR* ToString(ETextureDimension TextureDimension)
{
	switch (TextureDimension)
	{
	case ETextureDimension::Texture1D:        return "Texture1D";
	case ETextureDimension::Texture1DArray:   return "Texture1DArray";
	case ETextureDimension::Texture2D:        return "Texture2D";
	case ETextureDimension::Texture2DArray:   return "Texture2DArray";
	case ETextureDimension::TextureCube:      return "TextureCube";
	case ETextureDimension::TextureCubeArray: return "TextureCubeArray";
	case ETextureDimension::Texture3D:        return "Texture3D";

	default: return "Unknown";
	}
}

NODISCARD constexpr bool IsTextureCube(ETextureDimension Dimension)
{
    return Dimension == ETextureDimension::TextureCube || Dimension == ETextureDimension::TextureCubeArray;
}

struct IRHITextureData
{
    virtual ~IRHITextureData() = default;

    virtual int64 GetMipRowPitch(uint32 MipLevel = 0)   const = 0;
    virtual int64 GetMipSlicePitch(uint32 MipLevel = 0) const = 0;
    virtual void* GetMipData(uint32 MipLevel = 0)       const = 0;
};

struct FRHITextureDesc
{
    NODISCARD static FRHITextureDesc CreateTexture1D(EFormat InFormat, uint32 InWidth, uint32 InNumMipLevels,
        ETextureUsageFlags InUsageFlags, const FClearValue& InClearValue = FClearValue())
    {
        return FRHITextureDesc(ETextureDimension::Texture1D, InFormat, FIntVector3(InWidth, 1, 0), 1, InNumMipLevels, 1, InUsageFlags, InClearValue);
    }

    NODISCARD static FRHITextureDesc CreateTexture1DArray(EFormat InFormat, uint32 InWidth, uint32 InArraySlices, uint32 InNumMipLevels,
        ETextureUsageFlags InUsageFlags, const FClearValue& InClearValue = FClearValue())
    {
        return FRHITextureDesc(ETextureDimension::Texture1DArray, InFormat, FIntVector3(InWidth, 1, 0), InArraySlices, InNumMipLevels, 1, InUsageFlags, InClearValue);
    }

    NODISCARD static FRHITextureDesc CreateTexture2D(EFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InNumMipLevels, uint32 InNumSamples,
        ETextureUsageFlags InUsageFlags, const FClearValue& InClearValue = FClearValue())
    {
        return FRHITextureDesc(ETextureDimension::Texture2D, InFormat, FIntVector3(InWidth, InHeight, 0), 1, InNumMipLevels, InNumSamples, InUsageFlags, InClearValue);
    }

    NODISCARD static FRHITextureDesc CreateTexture2DArray(EFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InArraySlices, uint32 InNumMipLevels,
        uint32 InNumSamples, ETextureUsageFlags InUsageFlags, const FClearValue& InClearValue = FClearValue())
    {
        return FRHITextureDesc(ETextureDimension::Texture2DArray, InFormat, FIntVector3(InWidth, InHeight, 0), InArraySlices, InNumMipLevels, InNumSamples, InUsageFlags, InClearValue);
    }

    NODISCARD static FRHITextureDesc CreateTextureCube(EFormat InFormat, uint32 InExtent, uint32 InNumMipLevels, uint32 InNumSamples, 
        ETextureUsageFlags InUsageFlags, const FClearValue& InClearValue = FClearValue())
    {
        return FRHITextureDesc(ETextureDimension::TextureCube, InFormat, FIntVector3(InExtent, InExtent, 0), 1, InNumMipLevels, InNumSamples, InUsageFlags, InClearValue);
    }

    NODISCARD static FRHITextureDesc CreateTextureCubeArray(EFormat InFormat, uint32 InExtent, uint32 InArraySlices, uint32 InNumMipLevels,
        uint32 InNumSamples, ETextureUsageFlags InUsageFlags, const FClearValue& InClearValue = FClearValue())
    {
        return FRHITextureDesc(ETextureDimension::TextureCubeArray, InFormat, FIntVector3(InExtent, InExtent, 0), InArraySlices, InNumMipLevels, InNumSamples, InUsageFlags, InClearValue);
    }

    NODISCARD static FRHITextureDesc CreateTexture3D(EFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InDepth, uint32 InNumMipLevels,
        uint32 InNumSamples, ETextureUsageFlags InUsageFlags, const FClearValue& InClearValue = FClearValue())
    {
        return FRHITextureDesc(ETextureDimension::Texture3D, InFormat, FIntVector3(InWidth, InHeight, InDepth), 1, InNumMipLevels, InNumSamples, InUsageFlags, InClearValue);
    }

    FRHITextureDesc() noexcept = default;

    FRHITextureDesc(ETextureDimension InDimension, EFormat InFormat, FIntVector3 InExtent, uint32 InNumArraySlices, uint32 InNumMipLevels,
        uint32 InNumSamples, ETextureUsageFlags InUsageFlags, const FClearValue& InClearValue = FClearValue()) noexcept
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

    NODISCARD constexpr bool IsTexture1D()        const { return (Dimension == ETextureDimension::Texture1D); }
    NODISCARD constexpr bool IsTexture1DArray()   const { return (Dimension == ETextureDimension::Texture1DArray); }
    NODISCARD constexpr bool IsTexture2D()        const { return (Dimension == ETextureDimension::Texture2D); }
    NODISCARD constexpr bool IsTexture2DArray()   const { return (Dimension == ETextureDimension::Texture2DArray); }
    NODISCARD constexpr bool IsTextureCube()      const { return (Dimension == ETextureDimension::TextureCube); }
    NODISCARD constexpr bool IsTextureCubeArray() const { return (Dimension == ETextureDimension::TextureCubeArray); }
    NODISCARD constexpr bool IsTexture3D()        const { return (Dimension == ETextureDimension::Texture3D); }

    NODISCARD constexpr bool IsShaderResourceTexture()  const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::ShaderResourceTexture); }
    NODISCARD constexpr bool IsUnorderedAccessTexture() const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::UnorderedAccessTexture); }
    NODISCARD constexpr bool IsRenderTarget()           const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::RenderTarget); }
    NODISCARD constexpr bool IsDepthStencil()           const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::DepthStencil); }
    NODISCARD constexpr bool IsPresentable()            const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::Presentable); }
    NODISCARD constexpr bool IsShadingRateTexture()     const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::ShadingRateTexture); }
    NODISCARD constexpr bool IsNoDefaultSRV()           const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::NoDefaultSRV); }
    NODISCARD constexpr bool IsNoDefaultUAV()           const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::NoDefaultUAV); }
    NODISCARD constexpr bool IsMultisampled()           const { return (NumSamples > 1); }

    NODISCARD constexpr ETextureDimension  GetDimension()      const { return Dimension; }
    NODISCARD constexpr EFormat            GetFormat()         const { return Format; }
    NODISCARD constexpr ETextureUsageFlags GetUsageFlags()     const { return UsageFlags; }
    NODISCARD constexpr uint32             GetNumArraySlices() const { return NumArraySlices; }
    NODISCARD constexpr uint32             GetNumMipLevels()   const { return NumMipLevels; }
    NODISCARD constexpr uint32             GetNumSamples()     const { return NumSamples; }
    NODISCARD constexpr uint32             GetWidth()          const { return Extent.X; }
    NODISCARD constexpr uint32             GetHeight()         const { return Extent.Y; }
    NODISCARD constexpr uint32             GetDepth()          const { return Extent.Z; }
    NODISCARD constexpr const FIntVector3& GetExtent()         const { return Extent; }
    NODISCARD constexpr const FClearValue& GetClearValue()     const { return ClearValue; }

    bool operator==(const FRHITextureDesc& Other) const noexcept = default;

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
    explicit FRHITexture(const FRHITextureDesc& InTextureDesc)
        : FRHIResource()
        , Desc(InTextureDesc)
    {
    }

public:
    
    // Returns the native handle for this resource
    virtual void* GetRHINativeHandle() const { return nullptr; }

    virtual FRHIShaderResourceView*  GetShaderResourceView()  const { return nullptr; }
    virtual FRHIDescriptorHandle     GetBindlessSRVHandle()   const { return FRHIDescriptorHandle(); }
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const { return nullptr; }
    virtual FRHIDescriptorHandle     GetBindlessUAVHandle()   const { return FRHIDescriptorHandle(); }

    virtual void SetDebugName(const FString&) { }
    virtual FString GetDebugName() const { return ""; }

    const FRHITextureDesc& GetDesc() const
    {
        return Desc;
    }

    ETextureDimension GetDimension() const
    {
        return Desc.Dimension;
    }
    
    EFormat GetFormat() const
    {
        return Desc.Format;
    }
    
    ETextureUsageFlags GetFlags() const
    {
        return Desc.UsageFlags;
    }
    
    const FIntVector3& GetExtent() const
    {
        return Desc.Extent;
    }

    uint32 GetWidth() const
    {
        return Desc.Extent.X;
    }
    
    uint32 GetHeight() const
    {
        return Desc.Extent.Y;
    }
    
    uint32 GetDepth() const
    {
        return Desc.Extent.Z;
    }
    
    uint32 GetNumArraySlices() const
    {
        return Desc.NumArraySlices;
    }
    
    uint32 GetNumMipLevels() const
    {
        return Desc.NumMipLevels;
    }
    
    uint32 GetNumSamples() const
    {
        return Desc.NumSamples;
    }

protected:
    FRHITextureDesc Desc;
};
