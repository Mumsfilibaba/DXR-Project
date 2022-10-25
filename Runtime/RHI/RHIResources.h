#pragma once
#include "RHITypes.h"
#include "RHICore.h"

#include "Core/IRefCounted.h"
#include "Core/Math/Float.h"
#include "Core/Math/IntVector3.h"
#include "Core/Memory/Memory.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/String.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/NumericLimits.h"
#include "Core/Threading/AtomicInt.h"


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

typedef TSharedRef<class FRHIBuffer>              FRHIBufferRef;
typedef TSharedRef<class FRHITexture>             FRHITextureRef;
typedef TSharedRef<class FRHIViewport>            FRHIViewportRef;
typedef TSharedRef<class FRHIShaderResourceView>  FRHIShaderResourceViewRef;
typedef TSharedRef<class FRHIUnorderedAccessView> FRHIUnorderedAccessViewRef;
typedef TSharedRef<class FRHISamplerState>        FRHISamplerStateRef;
typedef TSharedRef<class FRHITimestampQuery>      FRHITimestampQueryRef;


class RHI_API FRHIResource
	: public IRefCounted
{
protected:
	FRHIResource()
		: StrongReferences(1)
	{ }

	virtual ~FRHIResource() = default;

public:
	virtual int32 AddRef() override
	{
		CHECK(StrongReferences.Load() > 0);
		++StrongReferences;
		return StrongReferences.Load();
	}

	virtual int32 Release() override
	{
		const int32 RefCount = --StrongReferences;
		CHECK(RefCount >= 0);

		if (RefCount < 1)
		{
			delete this;
		}

		return RefCount;
	}

	virtual int32 GetRefCount() const override
	{
		return StrongReferences.Load();
	}

protected:
	mutable FAtomicInt32 StrongReferences;
};


enum class EBufferUsageFlags : uint16
{
    None = 0,

    Default  = FLAG(1), // Default Device Memory
    Dynamic  = FLAG(2), // Dynamic Memory (D3D12 UploadHeap)
    ReadBack = FLAG(3), // Read-Back from GPU

    ConstantBuffer  = FLAG(4), // Can be used as ConstantBuffer
    UnorderedAccess = FLAG(5), // Can be used in UnorderedAccessViews
    ShaderResource  = FLAG(6), // Can be used in ShaderResourceViews
    VertexBuffer    = FLAG(7), // Can be used as VertexBuffer
    IndexBuffer     = FLAG(8), // Can be used as IndexBuffer

    RWBuffer = UnorderedAccess | ShaderResource
};

ENUM_CLASS_OPERATORS(EBufferUsageFlags);


struct FRHIBufferDesc
{
    FRHIBufferDesc()
        : Size(0)
        , Stride(0)
        , UsageFlags(EBufferUsageFlags::None)
    { }

    FRHIBufferDesc(
        uint64 InSize,
        uint32 InStride,
        EBufferUsageFlags InUsageFlags)
        : Size(InSize)
        , Stride(InStride)
        , UsageFlags(InUsageFlags)
    { }

    bool IsDefault()  const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::Default); }
    bool IsDynamic()  const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::Dynamic); }
    bool IsReadBack() const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::ReadBack); }
    
    bool IsConstantBuffer() const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::ConstantBuffer); }
    bool IsShaderResource() const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::ShaderResource); }
    bool IsVertexBuffer()   const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::VertexBuffer); }
    bool IsIndexBuffer()    const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::IndexBuffer); }
    
    bool IsUnorderedAccess() const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::UnorderedAccess); }

    bool operator==(const FRHIBufferDesc& Other) const
    {
        return (Size == Other.Size) && (Stride == Other.Stride) && (UsageFlags == Other.UsageFlags);
    }

    bool operator!=(const FRHIBufferDesc& Other) const
    {
        return !(*this == Other);
    }

    uint64            Size;
    uint32            Stride;
    EBufferUsageFlags UsageFlags;
};


class FRHIBuffer 
    : public FRHIResource
{
protected:
    explicit FRHIBuffer(const FRHIBufferDesc& InDesc)
        : FRHIResource()
        , Desc(InDesc)
    { }

public:

    /** @return - Returns a pointer to the RHI implementation of RHIBuffer */
    virtual void* GetRHIBaseBuffer() { return nullptr; }

    /** @return - Returns the native resource-handle */
    virtual void* GetRHIBaseResource() const { return nullptr; }

    /** @return - Returns a ConstantBuffer Bindless-handle */
    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }

    /** @return - Returns the name of the resource */
    virtual FString GetName() const { return ""; }

    /**
     * @brief        - Set the name of the resource
     * @param InName - New name of the resource
     */
    virtual void SetName(const FString& InName) { }
    
    /** @return - Returns the size of the buffer */
    FORCEINLINE uint64 GetSize() const { return Desc.Size; }
    
    /** @return - Returns the stride of each element in the buffer */
    FORCEINLINE uint32 GetStride() const { return Desc.Stride; }

    /** @return - Returns the flags of the buffer */
    FORCEINLINE EBufferUsageFlags GetFlags() const { return Desc.UsageFlags; }

    /** @return - Returns the description used to create the buffer */
    FORCEINLINE const FRHIBufferDesc& GetDesc() const { return Desc; }

protected:
    FRHIBufferDesc Desc;
};


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
    
    /** @return - Returns the number or MipLevels */
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


struct FRHIViewportDesc
{
    FRHIViewportDesc()
        : WindowHandle(nullptr)
        , ColorFormat(EFormat::Unknown)
        , DepthFormat(EFormat::Unknown)
        , Width(0)
        , Height(0)
    { }

    FRHIViewportDesc(
        void*   InWindowHandle,
        EFormat InColorFormat,
        EFormat InDepthFormat,
        uint16  InWidth,
        uint16  InHeight)
        : WindowHandle(InWindowHandle)
        , ColorFormat(InColorFormat)
        , DepthFormat(InDepthFormat)
        , Width(InWidth)
        , Height(InHeight)
    { }

    bool operator==(const FRHIViewportDesc& RHS) const
    {
        return (WindowHandle == RHS.WindowHandle)
            && (ColorFormat  == RHS.ColorFormat)
            && (DepthFormat  == RHS.DepthFormat)
            && (Width        == RHS.Width)
            && (Height       == RHS.Height);
    }

    bool operator!=(const FRHIViewportDesc& RHS) const
    {
        return !(*this == RHS);
    }

    void*   WindowHandle;

    EFormat ColorFormat;
    EFormat DepthFormat;

    uint16  Width;
    uint16  Height;
};


class FRHIViewport 
    : public FRHIResource
{
protected:
    explicit FRHIViewport(const FRHIViewportDesc& InDesc)
        : FRHIResource()
        , Desc(InDesc)
    { }

public:

    /**
     * @breif          - Resizes the BackBuffers of the Viewport
     * @param InWidth  - The new Width of the Viewport
     * @param InHeight - The new Height of the Viewport
     * @return         - Returns True if the resizing was successful 
     */
    virtual bool Resize(uint32 InWidth, uint32 InHeight) { return true; }

    /** @return - Returns the Texture representing the BackBuffer */
    virtual FRHITexture* GetBackBuffer() const { return nullptr; };

    /** @return - Returns the ColorFormat */
    FORCEINLINE EFormat GetColorFormat() const { return Desc.ColorFormat; }
    
    /** @return - Returns the DepthFormat */
    FORCEINLINE EFormat GetDepthFormat() const { return Desc.DepthFormat; }

    /** @return - Returns the width */
    FORCEINLINE uint32 GetWidth() const { return Desc.Width;  }
    
    /** @return - Returns the Height */
    FORCEINLINE uint32 GetHeight() const { return Desc.Height; }

	/** @return - Returns the Description */
	FORCEINLINE const FRHIViewportDesc& GetDesc() const { return Desc; }

protected:
    FRHIViewportDesc Desc;
};


enum class ESamplerMode : uint8
{
    Unknown    = 0,
    Wrap       = 1,
    Mirror     = 2,
    Clamp      = 3,
    Border     = 4,
    MirrorOnce = 5,
};

CONSTEXPR const CHAR* ToString(ESamplerMode SamplerMode)
{
    switch (SamplerMode)
    {
    case ESamplerMode::Wrap:       return "Wrap";
    case ESamplerMode::Mirror:     return "Mirror";
    case ESamplerMode::Clamp:      return "Clamp";
    case ESamplerMode::Border:     return "Border";
    case ESamplerMode::MirrorOnce: return "MirrorOnce";
    default:                       return "Unknown";
    }
}

enum class ESamplerFilter : uint8
{
    Unknown                                 = 0,
    MinMagMipPoint                          = 1,
    MinMagPoint_MipLinear                   = 2,
    MinPoint_MagLinear_MipPoint             = 3,
    MinPoint_MagMipLinear                   = 4,
    MinLinear_MagMipPoint                   = 5,
    MinLinear_MagPoint_MipLinear            = 6,
    MinMagLinear_MipPoint                   = 7,
    MinMagMipLinear                         = 8,
    Anistrotopic                            = 9,
    Comparison_MinMagMipPoint               = 10,
    Comparison_MinMagPoint_MipLinear        = 11,
    Comparison_MinPoint_MagLinear_MipPoint  = 12,
    Comparison_MinPoint_MagMipLinear        = 13,
    Comparison_MinLinear_MagMipPoint        = 14,
    Comparison_MinLinear_MagPoint_MipLinear = 15,
    Comparison_MinMagLinear_MipPoint        = 16,
    Comparison_MinMagMipLinear              = 17,
    Comparison_Anistrotopic                 = 18,
};

CONSTEXPR const CHAR* ToString(ESamplerFilter SamplerFilter)
{
    switch (SamplerFilter)
    {
    case ESamplerFilter::MinMagMipPoint:                          return "MinMagMipPoint";
    case ESamplerFilter::MinMagPoint_MipLinear:                   return "MinMagPoint_MipLinear";
    case ESamplerFilter::MinPoint_MagLinear_MipPoint:             return "MinPoint_MagLinear_MipPoint";
    case ESamplerFilter::MinPoint_MagMipLinear:                   return "MinPoint_MagMipLinear";
    case ESamplerFilter::MinLinear_MagMipPoint:                   return "MinLinear_MagMipPoint";
    case ESamplerFilter::MinLinear_MagPoint_MipLinear:            return "MinLinear_MagPoint_MipLinear";
    case ESamplerFilter::MinMagLinear_MipPoint:                   return "MinMagLinear_MipPoint";
    case ESamplerFilter::MinMagMipLinear:                         return "MinMagMipLinear";
    case ESamplerFilter::Anistrotopic:                            return "Anistrotopic";
    case ESamplerFilter::Comparison_MinMagMipPoint:               return "Comparison_MinMagMipPoint";
    case ESamplerFilter::Comparison_MinMagPoint_MipLinear:        return "Comparison_MinMagPoint_MipLinear";
    case ESamplerFilter::Comparison_MinPoint_MagLinear_MipPoint:  return "Comparison_MinPoint_MagLinear_MipPoint";
    case ESamplerFilter::Comparison_MinPoint_MagMipLinear:        return "Comparison_MinPoint_MagMipLinear";
    case ESamplerFilter::Comparison_MinLinear_MagMipPoint:        return "Comparison_MinLinear_MagMipPoint";
    case ESamplerFilter::Comparison_MinLinear_MagPoint_MipLinear: return "Comparison_MinLinear_MagPoint_MipLinear";
    case ESamplerFilter::Comparison_MinMagLinear_MipPoint:        return "Comparison_MinMagLinear_MipPoint";
    case ESamplerFilter::Comparison_MinMagMipLinear:              return "Comparison_MinMagMipLinear";
    case ESamplerFilter::Comparison_Anistrotopic:                 return "Comparison_Anistrotopic";
    default:                                                      return "Unknown";
    }
}


struct FRHISamplerStateDesc
{
    FRHISamplerStateDesc()
        : AddressU(ESamplerMode::Clamp)
        , AddressV(ESamplerMode::Clamp)
        , AddressW(ESamplerMode::Clamp)
        , Filter(ESamplerFilter::MinMagMipLinear)
        , ComparisonFunc(EComparisonFunc::Never)
        , MaxAnisotropy(1)
        , MipLODBias(0.0f)
        , MinLOD(TNumericLimits<float>::Lowest())
        , MaxLOD(TNumericLimits<float>::Max())
        , BorderColor()
    { }

    FRHISamplerStateDesc(ESamplerMode InAddressMode, ESamplerFilter InFilter)
        : AddressU(InAddressMode)
        , AddressV(InAddressMode)
        , AddressW(InAddressMode)
        , Filter(InFilter)
        , ComparisonFunc(EComparisonFunc::Unknown)
        , MaxAnisotropy(0)
        , MipLODBias(0.0f)
        , MinLOD(0.0f)
        , MaxLOD(TNumericLimits<float>::Max())
        , BorderColor(0.0f, 0.0f, 0.0f, 1.0f)
    { }

    FRHISamplerStateDesc(
        ESamplerMode       InAddressU,
        ESamplerMode       InAddressV,
        ESamplerMode       InAddressW,
        ESamplerFilter     InFilter,
        EComparisonFunc    InComparisonFunc,
        float              InMipLODBias,
        uint8              InMaxAnisotropy,
        float              InMinLOD,
        float              InMaxLOD,
        const FFloatColor& InBorderColor)
        : AddressU(InAddressU)
        , AddressV(InAddressV)
        , AddressW(InAddressW)
        , Filter(InFilter)
        , ComparisonFunc(InComparisonFunc)
        , MaxAnisotropy(InMaxAnisotropy)
        , MipLODBias(InMipLODBias)
        , MinLOD(InMinLOD)
        , MaxLOD(InMaxLOD)
        , BorderColor(InBorderColor)
    { }

    uint64 GetHash() const
    {
        uint64 Hash = ToUnderlying(AddressU);
        HashCombine(Hash, ToUnderlying(AddressV));
        HashCombine(Hash, ToUnderlying(AddressW));
        HashCombine(Hash, ToUnderlying(Filter));
        HashCombine(Hash, ToUnderlying(ComparisonFunc));
        HashCombine(Hash, MaxAnisotropy);
        HashCombine(Hash, MinLOD);
        HashCombine(Hash, MinLOD);
        HashCombine(Hash, MaxLOD);
        HashCombine(Hash, BorderColor.GetHash());
        return Hash;
    }

    bool operator==(const FRHISamplerStateDesc& RHS) const
    {
        return (AddressU       == RHS.AddressU)
            && (AddressV       == RHS.AddressV)
            && (AddressW       == RHS.AddressW)
            && (Filter         == RHS.Filter)
            && (ComparisonFunc == RHS.ComparisonFunc)
            && (MipLODBias     == RHS.MipLODBias)
            && (MaxAnisotropy  == RHS.MaxAnisotropy)
            && (MinLOD         == RHS.MinLOD)
            && (MaxLOD         == RHS.MaxLOD)
            && (BorderColor    == RHS.BorderColor);
    }

    bool operator!=(const FRHISamplerStateDesc& RHS) const
    {
        return !(*this == RHS);
    }

    ESamplerMode    AddressU;
    ESamplerMode    AddressV;
    ESamplerMode    AddressW;
    
    ESamplerFilter  Filter;

    EComparisonFunc ComparisonFunc;

    uint8           MaxAnisotropy;

    float           MipLODBias;

    float           MinLOD;
    float           MaxLOD;

    FFloatColor     BorderColor;
};


enum class EBufferSRVFormat : uint32
{
    None   = 0,
    Uint32 = 1,
};

CONSTEXPR const CHAR* ToString(EBufferSRVFormat BufferSRVFormat)
{
    switch (BufferSRVFormat)
    {
        case EBufferSRVFormat::Uint32: return "Uint32";
        default:                       return "Unknown";
    }
}

enum class EBufferUAVFormat : uint32
{
    None   = 0,
    Uint32 = 1,
};

CONSTEXPR const CHAR* ToString(EBufferUAVFormat BufferSRVFormat)
{
    switch (BufferSRVFormat)
    {
        case EBufferUAVFormat::Uint32: return "Uint32";
        default:                       return "Unknown";
    }
}

enum class EAttachmentLoadAction : uint8
{
    DontCare = 0, // Don't care
    Load     = 1, // Use the stored data when RenderPass begin
    Clear    = 2, // Clear data when RenderPass begin
};

CONSTEXPR const CHAR* ToString(EAttachmentLoadAction LoadAction)
{
    switch (LoadAction)
    {
        case EAttachmentLoadAction::DontCare: return "DontCare";
        case EAttachmentLoadAction::Load:     return "Load";
        case EAttachmentLoadAction::Clear:    return "Clear";
        default:                              return "Unknown";
    }
}

enum class EAttachmentStoreAction : uint8
{
    DontCare = 0, // Don't care
    Store    = 1, // Store the data after the RenderPass is finished
};

CONSTEXPR const CHAR* ToString(EAttachmentStoreAction StoreAction)
{
    switch (StoreAction)
    {
        case EAttachmentStoreAction::DontCare: return "DontCare";
        case EAttachmentStoreAction::Store:    return "Store";
        default:                               return "Unknown";
    }
}


struct FRHITextureSRVDesc
{
    FRHITextureSRVDesc()
        : Texture(nullptr)
        , MinLODClamp(0.0f)
        , Format(EFormat::Unknown)
        , FirstMipLevel(0)
        , NumMips(0)
        , FirstArraySlice(0)
        , NumSlices(0)
    { }

    FRHITextureSRVDesc(
        FRHITexture* InTexture,
        float        InMinLODClamp,
        EFormat      InFormat,
        uint8        InFirstMipLevel,
        uint8        InNumMips,
        uint16       InFirstArraySlice,
        uint16       InNumSlices)
        : Texture(InTexture)
        , MinLODClamp(InMinLODClamp)
        , Format(InFormat)
        , FirstMipLevel(InFirstMipLevel)
        , NumMips(InNumMips)
        , FirstArraySlice(InFirstArraySlice)
        , NumSlices(InNumSlices)
    { }

    uint64 GetHash() const
    {
        uint64 Hash = ToInteger(Texture);
        HashCombine(Hash, MinLODClamp);
        HashCombine(Hash, ToUnderlying(Format));
        HashCombine(Hash, FirstMipLevel);
        HashCombine(Hash, NumMips);
        HashCombine(Hash, FirstArraySlice);
        HashCombine(Hash, NumSlices);
        return Hash;
    }

    bool operator==(const FRHITextureSRVDesc& RHS) const
    {
        return (Texture         == RHS.Texture)
            && (MinLODClamp     == RHS.MinLODClamp)
            && (Format          == RHS.Format)
            && (FirstMipLevel   == RHS.FirstMipLevel)
            && (NumMips         == RHS.NumMips)
            && (FirstArraySlice == RHS.FirstArraySlice)
            && (NumSlices       == RHS.NumSlices);
    }

    bool operator!=(const FRHITextureSRVDesc& RHS) const
    {
        return !(*this == RHS);
    }

    FRHITexture* Texture;

    float        MinLODClamp;

    EFormat      Format;

    uint8        FirstMipLevel;
    uint8        NumMips;

    uint16       FirstArraySlice;
    uint16       NumSlices;
};


struct FRHIBufferSRVDesc
{
    FRHIBufferSRVDesc()
        : Buffer(nullptr)
        , Format(EBufferSRVFormat::None)
        , FirstElement(0)
        , NumElements(0)
    { }

    FRHIBufferSRVDesc(
        FRHIBuffer*      InBuffer,
        uint32           InFirstElement,
        uint32           InNumElements,
        EBufferSRVFormat InFormat = EBufferSRVFormat::None)
        : Buffer(InBuffer)
        , Format(InFormat)
        , FirstElement(InFirstElement)
        , NumElements(InNumElements)
    { }

    uint64 GetHash() const
    {
        uint64 Hash = ToInteger(Buffer);
        HashCombine(Hash, ToUnderlying(Format));
        HashCombine(Hash, FirstElement);
        HashCombine(Hash, NumElements);
        return Hash;
    }

    bool operator==(const FRHIBufferSRVDesc& RHS) const
    {
        return (Buffer       == RHS.Buffer) 
            && (Format       == RHS.Format)
            && (FirstElement == RHS.FirstElement) 
            && (NumElements  == RHS.NumElements);
    }

    bool operator!=(const FRHIBufferSRVDesc& RHS) const
    {
        return !(*this == RHS);
    }

    FRHIBuffer*      Buffer;

    EBufferSRVFormat Format;

    uint32           FirstElement;
    uint32           NumElements;
};


struct FRHITextureUAVDesc
{
    FRHITextureUAVDesc()
        : Texture(nullptr)
        , Format(EFormat::Unknown)
        , MipLevel(0)
        , FirstArraySlice(0)
        , NumSlices(0)
    { }

    FRHITextureUAVDesc(
        FRHITexture* InTexture,
        EFormat      InFormat,
        uint32       InMipLevel,
        uint32       InFirstArraySlice,
        uint32       InNumSlices)
        : Texture(InTexture)
        , Format(InFormat)
        , MipLevel(uint8(InMipLevel))
        , FirstArraySlice(uint16(InFirstArraySlice))
        , NumSlices(uint16(InNumSlices))
    { }

    FRHITextureUAVDesc(FRHITexture* InTexture, EFormat InFormat, uint32 InMipLevel)
        : Texture(InTexture)
        , Format(InFormat)
        , MipLevel(uint8(InMipLevel))
        , FirstArraySlice(0)
        , NumSlices(1)
    { }

    uint64 GetHash() const
    {
        uint64 Hash = ToInteger(Texture);
        HashCombine(Hash, ToUnderlying(Format));
        HashCombine(Hash, MipLevel);
        HashCombine(Hash, FirstArraySlice);
        HashCombine(Hash, NumSlices);
        return Hash;
    }

    bool operator==(const FRHITextureUAVDesc& RHS) const
    {
        return (Texture         == RHS.Texture)
            && (Format          == RHS.Format)
            && (MipLevel        == RHS.MipLevel)
            && (FirstArraySlice == RHS.FirstArraySlice)
            && (NumSlices       == RHS.NumSlices);
    }

    bool operator!=(const FRHITextureUAVDesc& RHS) const
    {
        return !(*this == RHS);
    }

    FRHITexture* Texture;

    EFormat      Format;

    uint8        MipLevel;

    uint16       FirstArraySlice;
    uint16       NumSlices;
};


struct FRHIBufferUAVDesc
{
    FRHIBufferUAVDesc()
        : Buffer(nullptr)
        , Format(EBufferUAVFormat::None)
        , FirstElement(0)
        , NumElements(0)
    { }

    FRHIBufferUAVDesc(
        FRHIBuffer*      InBuffer,
        uint32           InFirstElement,
        uint32           InNumElements,
        EBufferUAVFormat InFormat = EBufferUAVFormat::None)
        : Buffer(InBuffer)
        , Format(InFormat)
        , FirstElement(InFirstElement)
        , NumElements(InNumElements)
    { }

    uint64 GetHash() const
    {
        uint64 Hash = ToInteger(Buffer);
        HashCombine(Hash, ToUnderlying(Format));
        HashCombine(Hash, FirstElement);
        HashCombine(Hash, NumElements);
        return Hash;
    }

    bool operator==(const FRHIBufferUAVDesc& RHS) const
    {
        return (Buffer       == RHS.Buffer) 
            && (Format       == RHS.Format)
            && (FirstElement == RHS.FirstElement) 
            && (NumElements  == RHS.NumElements);
    }

    bool operator!=(const FRHIBufferUAVDesc& RHS) const
    {
        return !(*this == RHS);
    }

    FRHIBuffer*      Buffer;

    EBufferUAVFormat Format;

    uint32           FirstElement;
    uint32           NumElements;
};


class FRHIResourceView 
    : public FRHIResource
{
protected:
    explicit FRHIResourceView(FRHIResource* InResource)
        : FRHIResource()
        , Resource(InResource)
    { }

public:
    FRHIResource* GetResource() const { return Resource; }

protected:
    FRHIResource* Resource;
};


class FRHIShaderResourceView 
    : public FRHIResourceView
{
protected:
    explicit FRHIShaderResourceView(FRHIResource* InResource)
        : FRHIResourceView(InResource)
    { }

public:
    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }
};


class FRHIUnorderedAccessView 
    : public FRHIResourceView
{
protected:
    explicit FRHIUnorderedAccessView(FRHIResource* InResource)
        : FRHIResourceView(InResource)
    { }

public:
    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }
};


struct FRHIRenderTargetView
{
    FRHIRenderTargetView()
        : Texture(nullptr)
        , Format(EFormat::Unknown)
        , ArrayIndex(0)
        , MipLevel(0)
        , LoadAction(EAttachmentLoadAction::DontCare)
        , StoreAction(EAttachmentStoreAction::DontCare)
        , ClearValue()
    { }
    
    FRHIRenderTargetView(
        FRHITexture*           InTexture,
        EAttachmentLoadAction  InLoadAction  = EAttachmentLoadAction::Clear,
        EAttachmentStoreAction InStoreAction = EAttachmentStoreAction::Store,
        const FFloatColor&     InClearValue  = FFloatColor(0.0f, 0.0f, 0.0f, 1.0f))
        : Texture(InTexture)
        , Format(InTexture ? InTexture->GetFormat() : EFormat::Unknown)
        , ArrayIndex(0)
        , MipLevel(0)
        , LoadAction(InLoadAction)
        , StoreAction(InStoreAction)
        , ClearValue(InClearValue)
    { }

    FRHIRenderTargetView(
        FRHITexture*           InTexture,
        EFormat                InFormat,
        uint32                 InArrayIndex,
        uint32                 InMipLevel,
        EAttachmentLoadAction  InLoadAction,
        EAttachmentStoreAction InStoreAction,
        const FFloatColor&     InClearValue)
        : Texture(InTexture)
        , Format(InFormat)
        , ArrayIndex(uint16(InArrayIndex))
        , MipLevel(uint8(InMipLevel))
        , LoadAction(InLoadAction)
        , StoreAction(InStoreAction)
        , ClearValue(InClearValue)
    { }

    uint64 GetHash() const
    {
        uint64 Hash = ToInteger(Texture);
        HashCombine(Hash, ToUnderlying(Format));
        HashCombine(Hash, ArrayIndex);
        HashCombine(Hash, MipLevel);
        HashCombine(Hash, ToUnderlying(LoadAction));
        HashCombine(Hash, ToUnderlying(StoreAction));
        HashCombine(Hash, ClearValue.GetHash());
        return Hash;
    }

    bool operator==(const FRHIRenderTargetView& RHS) const
    {
        return (Texture     == RHS.Texture)
            && (Format      == RHS.Format)
            && (ArrayIndex  == RHS.ArrayIndex)
            && (MipLevel    == RHS.MipLevel)
            && (LoadAction  == RHS.LoadAction)
            && (StoreAction == RHS.StoreAction)
            && (ClearValue  == RHS.ClearValue);
    }

    bool operator!=(const FRHIRenderTargetView& RHS) const
    {
        return !(*this == RHS);
    }

    FRHITexture*           Texture;

    EFormat                Format;

    uint16                 ArrayIndex;
    uint8                  MipLevel;

    EAttachmentLoadAction  LoadAction;
    EAttachmentStoreAction StoreAction;

    FFloatColor            ClearValue;
};


struct FRHIDepthStencilView
{
    FRHIDepthStencilView()
        : Texture(nullptr)
        , Format(EFormat::Unknown)
        , ArrayIndex(0)
        , MipLevel(0)
        , LoadAction(EAttachmentLoadAction::DontCare)
        , StoreAction(EAttachmentStoreAction::DontCare)
        , ClearValue()
    { }

    explicit FRHIDepthStencilView(
        FRHITexture*              InTexture,
        EAttachmentLoadAction     InLoadAction  = EAttachmentLoadAction::Clear,
        EAttachmentStoreAction    InStoreAction = EAttachmentStoreAction::Store,
        const FDepthStencilValue& InClearValue  = FDepthStencilValue(1.0f, 0))
        : Texture(InTexture)
        , Format(InTexture ? InTexture->GetFormat() : EFormat::Unknown)
        , ArrayIndex(0)
        , MipLevel(0)
        , LoadAction(InLoadAction)
        , StoreAction(InStoreAction)
        , ClearValue(InClearValue)
    { }

    FRHIDepthStencilView(
        FRHITexture*              InTexture,
        uint16                    InArrayIndex,
        uint8                     InMipLevel,
        EAttachmentLoadAction     InLoadAction  = EAttachmentLoadAction::Clear,
        EAttachmentStoreAction    InStoreAction = EAttachmentStoreAction::Store,
        const FDepthStencilValue& InClearValue  = FDepthStencilValue(1.0f, 0))
        : Texture(InTexture)
        , Format(InTexture ? InTexture->GetFormat() : EFormat::Unknown)
        , ArrayIndex(uint16(InArrayIndex))
        , MipLevel(uint8(InMipLevel))
        , LoadAction(InLoadAction)
        , StoreAction(InStoreAction)
        , ClearValue(InClearValue)
    { }

    FRHIDepthStencilView(
        FRHITexture*              InTexture,
        uint16                    InArrayIndex,
        uint8                     InMipLevel,
        EFormat                   InFormat,
        EAttachmentLoadAction     InLoadAction  = EAttachmentLoadAction::Clear, 
        EAttachmentStoreAction    InStoreAction = EAttachmentStoreAction::Store,
        const FDepthStencilValue& InClearValue  = FDepthStencilValue(1.0f, 0))
        : Texture(InTexture)
        , Format(InFormat)
        , ArrayIndex(uint16(InArrayIndex))
        , MipLevel(uint8(InMipLevel))
        , LoadAction(InLoadAction)
        , StoreAction(InStoreAction)
        , ClearValue(InClearValue)
    { }

    uint64 GetHash() const
    {
        uint64 Hash = ToInteger(Texture);
        HashCombine(Hash, ToUnderlying(Format));
        HashCombine(Hash, ArrayIndex);
        HashCombine(Hash, MipLevel);
        HashCombine(Hash, ToUnderlying(LoadAction));
        HashCombine(Hash, ToUnderlying(StoreAction));
        HashCombine(Hash, ClearValue.GetHash());
        return Hash;
    }

    bool operator==(const FRHIDepthStencilView& RHS) const
    {
        return (Texture     == RHS.Texture)
            && (Format      == RHS.Format)
            && (ArrayIndex  == RHS.ArrayIndex)
            && (MipLevel    == RHS.MipLevel)
            && (LoadAction  == RHS.LoadAction)
            && (StoreAction == RHS.StoreAction)
            && (ClearValue  == RHS.ClearValue);
    }

    bool operator!=(const FRHIDepthStencilView& RHS) const
    {
        return !(*this == RHS);
    }

    FRHITexture*           Texture;

    EFormat                Format;

    uint16                 ArrayIndex;
    uint8                  MipLevel;

    EAttachmentLoadAction  LoadAction;
    EAttachmentStoreAction StoreAction;

    FDepthStencilValue     ClearValue;
};


typedef TStaticArray<FRHIRenderTargetView, kRHIMaxRenderTargetCount> FRHIRenderTargetViewArray;

struct FRHIRenderPassDesc
{
    FRHIRenderPassDesc()
        : ShadingRateTexture(nullptr)
        , DepthStencilView()
        , StaticShadingRate(EShadingRate::VRS_1x1)
        , NumRenderTargets(0)
        , RenderTargets()
    { }

    FRHIRenderPassDesc(
        const FRHIRenderTargetViewArray& InRenderTargets,
        uint32                           InNumRenderTargets,
        FRHIDepthStencilView             InDepthStencilView,
        FRHITexture*                     InShadingRateTexture = nullptr,
        EShadingRate                     InStaticShadingRate  = EShadingRate::VRS_1x1)
        : ShadingRateTexture(InShadingRateTexture)
        , DepthStencilView(InDepthStencilView)
        , StaticShadingRate(InStaticShadingRate)
        , NumRenderTargets(InNumRenderTargets)
        , RenderTargets(InRenderTargets)
    { }

    bool operator==(const FRHIRenderPassDesc& RHS) const
    {
        return (NumRenderTargets   == RHS.NumRenderTargets)
            && (RenderTargets      == RHS.RenderTargets)
            && (DepthStencilView   == RHS.DepthStencilView)
            && (ShadingRateTexture == RHS.ShadingRateTexture)
            && (StaticShadingRate  == RHS.StaticShadingRate);
    }

    bool operator!=(const FRHIRenderPassDesc& RHS) const
    {
        return !(*this == RHS);
    }

    FRHITexture*         ShadingRateTexture;

    FRHIDepthStencilView DepthStencilView;

    EShadingRate         StaticShadingRate;

    uint32                    NumRenderTargets;
    FRHIRenderTargetViewArray RenderTargets;
};


class FRHISamplerState 
    : public FRHIResource
{
protected:
    explicit FRHISamplerState(const FRHISamplerStateDesc& InDesc)
        : Desc(InDesc)
    { }

public:

    /** @return - Returns the Bindless descriptor-handle if the RHI-supports descriptor-handles */
    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }

    /** @return - Returns the Description */
    const FRHISamplerStateDesc& GetDesc() const { return Desc; } 

private:
    FRHISamplerStateDesc Desc;
};


struct FRHITimestamp
{
    uint64 Begin = 0;
    uint64 End   = 0;
};

class FRHITimestampQuery 
    : public FRHIResource
{
protected:
    FRHITimestampQuery() = default;

public:

    /**
     * @brief          - Retrieve a certain timestamp 
     * @param OutQuery - Structure to store the timestamp in
     * @param Index    - Index of the query to retrieve 
     */
    virtual void GetTimestampFromIndex(FRHITimestamp& OutQuery, uint32 Index) const = 0;

    /**
     * @brief  - Get the frequency of the queue that the query where used on 
     * @return - Returns the frequency of the query
     */
    virtual uint64 GetFrequency() const = 0;
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
