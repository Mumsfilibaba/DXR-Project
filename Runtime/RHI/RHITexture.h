#pragma once
#include "RHITypes.h"
#include "RHIResourceBase.h"

#include "Core/Math/IntVector3.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/String.h"
#include "Core/Templates/EnumUtilities.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

struct IRHITextureData;
class FRHIShaderResourceView;
class FRHIUnorderedAccessView;

typedef TSharedRef<class FRHITexture> FRHITextureRef;


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


struct IRHITextureData
{
    virtual ~IRHITextureData() = default;

    virtual void* GetMipData(uint32 MipLevel = 0) const = 0;

    virtual int64 GetMipRowPitch(uint32 MipLevel = 0)   const = 0;
    virtual int64 GetMipSlicePitch(uint32 MipLevel = 0) const = 0;
};


struct FRHITextureDesc
{
    static FRHITextureDesc CreateTexture2D(
        EFormat            InFormat,
        uint32             InWidth,
        uint32             InHeight,
        uint32             InNumMipLevels,
        uint32             InNumSamples,
        ETextureUsageFlags InUsageFlags,
        const FClearValue& InClearValue = FClearValue())
    {
        FRHITextureDesc NewTextureDesc(
            ETextureDimension::Texture2D,
            InFormat,
            FIntVector3(InWidth, InHeight, 0),
            1,
            InNumMipLevels,
            InNumSamples,
            InUsageFlags,
            InClearValue);

        return NewTextureDesc;
    }

    static FRHITextureDesc CreateTexture2DArray(
        EFormat            InFormat,
        uint32             InWidth,
        uint32             InHeight,
        uint32             InArraySlices,
        uint32             InNumMipLevels,
        uint32             InNumSamples,
        ETextureUsageFlags InUsageFlags,
        const FClearValue& InClearValue = FClearValue())
    {
        FRHITextureDesc NewTextureDesc(
            ETextureDimension::Texture2DArray,
            InFormat,
            FIntVector3(InWidth, InHeight, 0),
            InArraySlices,
            InNumMipLevels,
            InNumSamples,
            InUsageFlags,
            InClearValue);

        return NewTextureDesc;
    }

    static FRHITextureDesc CreateTextureCube(
        EFormat            InFormat,
        uint32             InExtent,
        uint32             InNumMipLevels,
        uint32             InNumSamples,
        ETextureUsageFlags InUsageFlags,
        const FClearValue& InClearValue = FClearValue())
    {
        FRHITextureDesc NewTextureDesc(
            ETextureDimension::TextureCube,
            InFormat,
            FIntVector3(InExtent, InExtent, 0),
            1,
            InNumMipLevels,
            InNumSamples,
            InUsageFlags,
            InClearValue);

        return NewTextureDesc;
    }

    static FRHITextureDesc CreateTextureCubeArray(
        EFormat            InFormat,
        uint32             InExtent,
        uint32             InArraySlices,
        uint32             InNumMipLevels,
        uint32             InNumSamples,
        ETextureUsageFlags InUsageFlags,
        const FClearValue& InClearValue = FClearValue())
    {
        FRHITextureDesc NewTextureDesc(
            ETextureDimension::TextureCubeArray,
            InFormat,
            FIntVector3(InExtent, InExtent, 0),
            InArraySlices,
            InNumMipLevels,
            InNumSamples,
            InUsageFlags,
            InClearValue);

        return NewTextureDesc;
    }

    static FRHITextureDesc CreateTexture3D(
        EFormat            InFormat,
        uint32             InWidth,
        uint32             InHeight,
        uint32             InDepth,
        uint32             InNumMipLevels,
        uint32             InNumSamples,
        ETextureUsageFlags InUsageFlags,
        const FClearValue& InClearValue = FClearValue())
    {
        FRHITextureDesc NewTextureDesc(
            ETextureDimension::Texture3D,
            InFormat,
            FIntVector3(InWidth, InHeight, InDepth),
            1,
            InNumMipLevels,
            InNumSamples,
            InUsageFlags,
            InClearValue);

        return NewTextureDesc;
    }

    FRHITextureDesc()
        : Dimension(ETextureDimension::None)
        , Format(EFormat::Unknown)
        , UsageFlags(ETextureUsageFlags::None)
        , Extent()
        , NumArraySlices(0)
        , NumMipLevels(0)
        , NumSamples(0)
        , ClearValue()
    { }

    FRHITextureDesc(
        ETextureDimension  InDimension,
        EFormat            InFormat,
        FIntVector3        InExtent,
        uint32             InNumArraySlices,
        uint32             InNumMipLevels,
        uint32             InNumSamples,
        ETextureUsageFlags InUsageFlags,
        const FClearValue& InClearValue  = FClearValue())
        : Dimension(InDimension)
        , Format(InFormat)
        , UsageFlags(InUsageFlags)
        , Extent(InExtent)
        , NumArraySlices(InNumArraySlices)
        , NumMipLevels(InNumMipLevels)
        , NumSamples(InNumSamples)
        , ClearValue(InClearValue)
    { }

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

    bool operator==(const FRHITextureDesc& RHS) const
    {
        return (Dimension      == RHS.Dimension)
            && (Format         == RHS.Format)
            && (UsageFlags     == RHS.UsageFlags)
            && (Extent         == RHS.Extent)
            && (NumArraySlices == RHS.NumArraySlices)
            && (NumMipLevels   == RHS.NumMipLevels)
            && (NumSamples     == RHS.NumSamples)
            && (ClearValue     == RHS.ClearValue);
    }

    bool operator!=(const FRHITextureDesc& RHS) const
    {
        return !(*this == RHS);
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


class FRHITexture 
    : public FRHIResource
{
protected:
    explicit FRHITexture(const FRHITextureDesc& InDesc)
        : FRHIResource()
        , Desc(InDesc)
    { }

public:

    /** @return - Returns a pointer to the RHI implementation of RHITexture */
    virtual void* GetRHIBaseTexture() { return nullptr; }

    /** @return - Returns the native resource-handle */
    virtual void* GetRHIBaseResource() const { return nullptr; }

    /** @return - Returns the default ShaderResourceView */
    virtual FRHIShaderResourceView* GetShaderResourceView() const { return nullptr; }

    /** @return - Returns the default UnorderedAccessView */
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const { return nullptr; }

    /** @return - Returns a Bindless-handle to the default ShaderResourceView */
    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const { return FRHIDescriptorHandle(); }

    /** @return - Returns a Bindless-handle to the default UnorderedAccessView */
    virtual FRHIDescriptorHandle GetBindlessUAVHandle() const { return FRHIDescriptorHandle(); }
    
    /** @return - Returns the dimension of the texture */
    FORCEINLINE ETextureDimension GetDimension() const { return Desc.Dimension; }
    
    /** @return - Returns the format of the texture */
    FORCEINLINE EFormat GetFormat() const { return Desc.Format; }

    /** @return - Returns the flags of the texture */
    FORCEINLINE ETextureUsageFlags GetFlags() const { return Desc.UsageFlags; }
    
    /** @return - Returns the extent */
    FORCEINLINE const FIntVector3& GetExtent() const { return Desc.Extent; }
    
    /** @return - Returns the width */
    FORCEINLINE uint32 GetWidth() const { return Desc.Extent.x; }
    
    /** @return - Returns the height */
    FORCEINLINE uint32 GetHeight() const { return Desc.Extent.y; }

    /** @return - Returns the depth */
    FORCEINLINE uint32 GetDepth()  const { return Desc.Extent.z; }

    /** @return - Returns the number of array-slices */
    FORCEINLINE uint32 GetNumArraySlices() const { return Desc.NumArraySlices; }
    
    /** @return - Returns the number or mip-levels */
    FORCEINLINE uint32 GetNumMipLevels() const { return Desc.NumMipLevels; }
    
    /** @return - Returns the number of samples */
    FORCEINLINE uint32 GetNumSamples() const { return Desc.NumSamples; }
    
    /** @return - Returns the description of the texture */
    FORCEINLINE const FRHITextureDesc& GetDesc() const { return Desc; }

    /** @return - Returns the name of the resource */
    virtual FString GetName() const { return ""; }

    /**
     * @brief        - Set the name of the resource
     * @param InName - New name of the resource
     */
    virtual void SetName(const FString& InName) { }

protected:
    FRHITextureDesc Desc;
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
