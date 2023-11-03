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

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FRHIShader;
class FRHIVertexShader;
class FRHIHullShader;
class FRHIDomainShader;
class FRHIGeometryShader;
class FRHIPixelShader;
class FRHIMeshShader;
class FRHIAmplificationShader;
class FRHIComputeShader;
class FRHIRayTracingShader;
class FRHIRayGenShader;
class FRHIRayCallableShader;
class FRHIRayMissShader;
class FRHIRayAnyHitShader;
class FRHIRayClosestHitShader;
class FRHIShaderResourceView;
class FRHIUnorderedAccessView;
struct IRHITextureData;

typedef TSharedRef<class FRHIBuffer>                  FRHIBufferRef;
typedef TSharedRef<class FRHITexture>                 FRHITextureRef;
typedef TSharedRef<FRHIShaderResourceView>            FRHIShaderResourceViewRef;
typedef TSharedRef<FRHIUnorderedAccessView>           FRHIUnorderedAccessViewRef;
typedef TSharedRef<class FRHISamplerState>            FRHISamplerStateRef;
typedef TSharedRef<class FRHIViewport>                FRHIViewportRef;
typedef TSharedRef<class FRHITimestampQuery>          FRHITimestampQueryRef;
typedef TSharedRef<class FRHIRasterizerState>         FRHIRasterizerStateRef;
typedef TSharedRef<class FRHIBlendState>              FRHIBlendStateRef;
typedef TSharedRef<class FRHIDepthStencilState>       FRHIDepthStencilStateRef;
typedef TSharedRef<class FRHIVertexInputLayout>       FRHIVertexInputLayoutRef;
typedef TSharedRef<class FRHIGraphicsPipelineState>   FRHIGraphicsPipelineStateRef;
typedef TSharedRef<class FRHIComputePipelineState>    FRHIComputePipelineStateRef;
typedef TSharedRef<class FRHIRayTracingPipelineState> FRHIRayTracingPipelineStateRef;


class RHI_API FRHIResource : public IRefCounted
{
protected:
    FRHIResource()
        : StrongReferences(1)
    {
    }

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
    {
    }

    FRHIBufferDesc(uint64 InSize, uint32 InStride, EBufferUsageFlags InUsageFlags)
        : Size(InSize)
        , Stride(InStride)
        , UsageFlags(InUsageFlags)
    {
    }

    bool IsDefault() const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::Default); }

    bool IsDynamic() const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::Dynamic); }
    
    bool IsReadBack() const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::ReadBack); }
    
    bool IsConstantBuffer() const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::ConstantBuffer); }
    
    bool IsShaderResource() const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::ShaderResource); }
    
    bool IsVertexBuffer() const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::VertexBuffer); }
    
    bool IsIndexBuffer() const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::IndexBuffer); }
    
    bool IsUnorderedAccess() const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::UnorderedAccess); }

    bool operator==(const FRHIBufferDesc& Other) const
    {
        return Size == Other.Size && Stride == Other.Stride && UsageFlags == Other.UsageFlags;
    }

    bool operator!=(const FRHIBufferDesc& Other) const
    {
        return !(*this == Other);
    }

    uint64            Size;
    uint32            Stride;
    EBufferUsageFlags UsageFlags;
};

class FRHIBuffer : public FRHIResource
{
protected:
    explicit FRHIBuffer(const FRHIBufferDesc& InDesc)
        : FRHIResource()
        , Desc(InDesc)
    {
    }

    virtual ~FRHIBuffer() = default;

public:
    virtual void* GetRHIBaseBuffer() { return nullptr; }

    virtual void* GetRHIBaseResource() const { return nullptr; }

    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }

    virtual void SetName(const FString& InName) { }

    virtual FString GetName() const { return ""; }
    
    uint64 GetSize() const
    {
        return Desc.Size;
    }
    
    uint32 GetStride() const
    {
        return Desc.Stride;
    }

    EBufferUsageFlags GetFlags() const
    {
        return Desc.UsageFlags;
    }

    const FRHIBufferDesc& GetDesc() const
    {
        return Desc;
    }

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

constexpr bool IsTextureCube(ETextureDimension Dimension)
{
    return Dimension == ETextureDimension::TextureCube || Dimension == ETextureDimension::TextureCubeArray;
}


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
        return FRHITextureDesc(
            ETextureDimension::Texture2D,
            InFormat,
            FIntVector3(InWidth, InHeight, 0),
            1,
            InNumMipLevels,
            InNumSamples,
            InUsageFlags,
            InClearValue);
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
        return FRHITextureDesc(
            ETextureDimension::Texture2DArray,
            InFormat,
            FIntVector3(InWidth, InHeight, 0),
            InArraySlices,
            InNumMipLevels,
            InNumSamples,
            InUsageFlags,
            InClearValue);
    }

    static FRHITextureDesc CreateTextureCube(
        EFormat            InFormat,
        uint32             InExtent,
        uint32             InNumMipLevels,
        uint32             InNumSamples,
        ETextureUsageFlags InUsageFlags,
        const FClearValue& InClearValue = FClearValue())
    {
        return FRHITextureDesc(
            ETextureDimension::TextureCube,
            InFormat,
            FIntVector3(InExtent, InExtent, 0),
            1,
            InNumMipLevels,
            InNumSamples,
            InUsageFlags,
            InClearValue);
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
        return FRHITextureDesc(
            ETextureDimension::TextureCubeArray,
            InFormat,
            FIntVector3(InExtent, InExtent, 0),
            InArraySlices,
            InNumMipLevels,
            InNumSamples,
            InUsageFlags,
            InClearValue);
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
        return FRHITextureDesc(
            ETextureDimension::Texture3D,
            InFormat,
            FIntVector3(InWidth, InHeight, InDepth),
            1,
            InNumMipLevels,
            InNumSamples,
            InUsageFlags,
            InClearValue);
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
    {
    }

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
    {
    }

    bool IsTexture2D() const { return (Dimension == ETextureDimension::Texture2D); }
    
    bool IsTexture2DArray() const { return (Dimension == ETextureDimension::Texture2DArray); }
    
    bool IsTextureCube() const { return (Dimension == ETextureDimension::TextureCube); }
    
    bool IsTextureCubeArray() const { return (Dimension == ETextureDimension::TextureCubeArray); }
    
    bool IsTexture3D() const { return (Dimension == ETextureDimension::Texture3D); }

    bool IsShaderResource() const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::ShaderResource); }
    
    bool IsUnorderedAccess() const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::UnorderedAccess); }
    
    bool IsRenderTarget() const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::RenderTarget); }
    
    bool IsDepthStencil() const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::DepthStencil); }
    
    bool IsPresentable() const { return IsEnumFlagSet(UsageFlags, ETextureUsageFlags::Presentable); }

    bool IsMultisampled() const { return (NumSamples > 1); }

    bool operator==(const FRHITextureDesc& Other) const
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

    bool operator!=(const FRHITextureDesc& Other) const
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
    explicit FRHITexture(const FRHITextureDesc& InDesc)
        : FRHIResource()
        , Desc(InDesc)
    {
    }

    virtual ~FRHITexture() = default;

public:
    virtual void* GetRHIBaseTexture() { return nullptr; }

    virtual void* GetRHIBaseResource() const { return nullptr; }

    virtual FRHIShaderResourceView* GetShaderResourceView() const { return nullptr; }

    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const { return nullptr; }

    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const { return FRHIDescriptorHandle(); }

    virtual FRHIDescriptorHandle GetBindlessUAVHandle() const { return FRHIDescriptorHandle(); }

    virtual void SetName(const FString& InName) { }

    virtual FString GetName() const { return ""; }

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
        return Desc.Extent.x;
    }
    
    uint32 GetHeight() const
    {
        return Desc.Extent.y;
    }

    uint32 GetDepth() const
    {
        return Desc.Extent.z;
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
    
    const FRHITextureDesc& GetDesc() const
    {
        return Desc;
    }

protected:
    FRHITextureDesc Desc;
};


enum class EBufferSRVFormat : uint32
{
    None   = 0,
    Uint32 = 1,
};

constexpr const CHAR* ToString(EBufferSRVFormat BufferSRVFormat)
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

constexpr const CHAR* ToString(EBufferUAVFormat BufferSRVFormat)
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

constexpr const CHAR* ToString(EAttachmentLoadAction LoadAction)
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

constexpr const CHAR* ToString(EAttachmentStoreAction StoreAction)
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
    {
    }

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
    {
    }

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

    bool operator==(const FRHITextureSRVDesc& Other) const
    {
        return Texture         == Other.Texture
            && MinLODClamp     == Other.MinLODClamp
            && Format          == Other.Format
            && FirstMipLevel   == Other.FirstMipLevel
            && NumMips         == Other.NumMips
            && FirstArraySlice == Other.FirstArraySlice
            && NumSlices       == Other.NumSlices;
    }

    bool operator!=(const FRHITextureSRVDesc& Other) const
    {
        return !(*this == Other);
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
    {
    }

    FRHIBufferSRVDesc(FRHIBuffer* InBuffer, uint32 InFirstElement, uint32 InNumElements, EBufferSRVFormat InFormat = EBufferSRVFormat::None)
        : Buffer(InBuffer)
        , Format(InFormat)
        , FirstElement(InFirstElement)
        , NumElements(InNumElements)
    {
    }

    uint64 GetHash() const
    {
        uint64 Hash = ToInteger(Buffer);
        HashCombine(Hash, ToUnderlying(Format));
        HashCombine(Hash, FirstElement);
        HashCombine(Hash, NumElements);
        return Hash;
    }

    bool operator==(const FRHIBufferSRVDesc& Other) const
    {
        return Buffer       == Other.Buffer 
            && Format       == Other.Format
            && FirstElement == Other.FirstElement 
            && NumElements  == Other.NumElements;
    }

    bool operator!=(const FRHIBufferSRVDesc& Other) const
    {
        return !(*this == Other);
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
    {
    }

    FRHITextureUAVDesc(FRHITexture* InTexture, EFormat InFormat, uint32 InMipLevel, uint32 InFirstArraySlice, uint32 InNumSlices)
        : Texture(InTexture)
        , Format(InFormat)
        , MipLevel(uint8(InMipLevel))
        , FirstArraySlice(uint16(InFirstArraySlice))
        , NumSlices(uint16(InNumSlices))
    {
    }

    FRHITextureUAVDesc(FRHITexture* InTexture, EFormat InFormat, uint32 InMipLevel)
        : Texture(InTexture)
        , Format(InFormat)
        , MipLevel(uint8(InMipLevel))
        , FirstArraySlice(0)
        , NumSlices(1)
    {
    }

    uint64 GetHash() const
    {
        uint64 Hash = ToInteger(Texture);
        HashCombine(Hash, ToUnderlying(Format));
        HashCombine(Hash, MipLevel);
        HashCombine(Hash, FirstArraySlice);
        HashCombine(Hash, NumSlices);
        return Hash;
    }

    bool operator==(const FRHITextureUAVDesc& Other) const
    {
        return Texture         == Other.Texture
            && Format          == Other.Format
            && MipLevel        == Other.MipLevel
            && FirstArraySlice == Other.FirstArraySlice
            && NumSlices       == Other.NumSlices;
    }

    bool operator!=(const FRHITextureUAVDesc& Other) const
    {
        return !(*this == Other);
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
    {
    }

    FRHIBufferUAVDesc(FRHIBuffer* InBuffer, uint32 InFirstElement, uint32 InNumElements, EBufferUAVFormat InFormat = EBufferUAVFormat::None)
        : Buffer(InBuffer)
        , Format(InFormat)
        , FirstElement(InFirstElement)
        , NumElements(InNumElements)
    {
    }

    uint64 GetHash() const
    {
        uint64 Hash = ToInteger(Buffer);
        HashCombine(Hash, ToUnderlying(Format));
        HashCombine(Hash, FirstElement);
        HashCombine(Hash, NumElements);
        return Hash;
    }

    bool operator==(const FRHIBufferUAVDesc& Other) const
    {
        return Buffer       == Other.Buffer 
            && Format       == Other.Format
            && FirstElement == Other.FirstElement 
            && NumElements  == Other.NumElements;
    }

    bool operator!=(const FRHIBufferUAVDesc& Other) const
    {
        return !(*this == Other);
    }

    FRHIBuffer*      Buffer;
    EBufferUAVFormat Format;
    uint32           FirstElement;
    uint32           NumElements;
};


class FRHIResourceView : public FRHIResource
{
protected:
    explicit FRHIResourceView(FRHIResource* InResource)
        : FRHIResource()
        , Resource(InResource)
    {
    }

    virtual ~FRHIResourceView() = default;

public:
    FORCEINLINE FRHIResource* GetResource() const
    {
        return Resource;
    }

protected:
    FRHIResource* Resource;
};


class FRHIShaderResourceView : public FRHIResourceView
{
protected:
    explicit FRHIShaderResourceView(FRHIResource* InResource)
        : FRHIResourceView(InResource)
    {
    }

    virtual ~FRHIShaderResourceView() = default;

public:
    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }
};


class FRHIUnorderedAccessView : public FRHIResourceView
{
protected:
    explicit FRHIUnorderedAccessView(FRHIResource* InResource)
        : FRHIResourceView(InResource)
    {
    }

    virtual ~FRHIUnorderedAccessView() = default;

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
    {
    }
    
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
    {
    }

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
    {
    }

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

    bool operator==(const FRHIRenderTargetView& Other) const
    {
        return Texture     == Other.Texture
            && Format      == Other.Format
            && ArrayIndex  == Other.ArrayIndex
            && MipLevel    == Other.MipLevel
            && LoadAction  == Other.LoadAction
            && StoreAction == Other.StoreAction
            && ClearValue  == Other.ClearValue;
    }

    bool operator!=(const FRHIRenderTargetView& Other) const
    {
        return !(*this == Other);
    }

    FRHITexture*           Texture;
    FFloatColor            ClearValue;
    uint16                 ArrayIndex;
    EFormat                Format;
    uint8                  MipLevel;
    EAttachmentLoadAction  LoadAction;
    EAttachmentStoreAction StoreAction;
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
    {
    }

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
    {
    }

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
    {
    }

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
    {
    }

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

    bool operator==(const FRHIDepthStencilView& Other) const
    {
        return Texture     == Other.Texture
            && Format      == Other.Format
            && ArrayIndex  == Other.ArrayIndex
            && MipLevel    == Other.MipLevel
            && LoadAction  == Other.LoadAction
            && StoreAction == Other.StoreAction
            && ClearValue  == Other.ClearValue;
    }

    bool operator!=(const FRHIDepthStencilView& Other) const
    {
        return !(*this == Other);
    }

    FRHITexture*           Texture;
    FDepthStencilValue     ClearValue;
    EFormat                Format;
    uint16                 ArrayIndex;
    uint8                  MipLevel;
    EAttachmentLoadAction  LoadAction;
    EAttachmentStoreAction StoreAction;
};


struct FRHIRenderPassDesc
{
    typedef TStaticArray<FRHIRenderTargetView, FRHILimits::MaxRenderTargets> FRenderTargetViews;
    
    FRHIRenderPassDesc()
        : ShadingRateTexture(nullptr)
        , DepthStencilView()
        , StaticShadingRate(EShadingRate::VRS_1x1)
        , NumRenderTargets(0)
        , RenderTargets()
    {
    }

	FRHIRenderPassDesc(const FRenderTargetViews& InRenderTargets, uint32 InNumRenderTargets)
		: ShadingRateTexture(nullptr)
		, DepthStencilView()
		, StaticShadingRate(EShadingRate::VRS_1x1)
		, NumRenderTargets(InNumRenderTargets)
		, RenderTargets(InRenderTargets)
	{
	}

    FRHIRenderPassDesc(
        const FRenderTargetViews& InRenderTargets,
        uint32                    InNumRenderTargets,
        FRHIDepthStencilView      InDepthStencilView,
        FRHITexture*              InShadingRateTexture = nullptr,
        EShadingRate              InStaticShadingRate  = EShadingRate::VRS_1x1)
        : ShadingRateTexture(InShadingRateTexture)
        , DepthStencilView(InDepthStencilView)
        , StaticShadingRate(InStaticShadingRate)
        , NumRenderTargets(InNumRenderTargets)
        , RenderTargets(InRenderTargets)
    {
    }

    bool operator==(const FRHIRenderPassDesc& Other) const
    {
        return NumRenderTargets   == Other.NumRenderTargets
            && RenderTargets      == Other.RenderTargets
            && DepthStencilView   == Other.DepthStencilView
            && ShadingRateTexture == Other.ShadingRateTexture
            && StaticShadingRate  == Other.StaticShadingRate;
    }

    bool operator!=(const FRHIRenderPassDesc& Other) const
    {
        return !(*this == Other);
    }

    FRHITexture*         ShadingRateTexture;
    FRHIDepthStencilView DepthStencilView;
    EShadingRate         StaticShadingRate;
    uint32               NumRenderTargets;
    FRenderTargetViews   RenderTargets;
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

constexpr const CHAR* ToString(ESamplerMode SamplerMode)
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

constexpr const CHAR* ToString(ESamplerFilter SamplerFilter)
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
    {
    }

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
    {
    }

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
    {
    }

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

    bool operator==(const FRHISamplerStateDesc& Other) const
    {
        return AddressU       == Other.AddressU
            && AddressV       == Other.AddressV
            && AddressW       == Other.AddressW
            && Filter         == Other.Filter
            && ComparisonFunc == Other.ComparisonFunc
            && MipLODBias     == Other.MipLODBias
            && MaxAnisotropy  == Other.MaxAnisotropy
            && MinLOD         == Other.MinLOD
            && MaxLOD         == Other.MaxLOD
            && BorderColor    == Other.BorderColor;
    }

    bool operator!=(const FRHISamplerStateDesc& Other) const
    {
        return !(*this == Other);
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


class FRHISamplerState : public FRHIResource
{
protected:
    explicit FRHISamplerState(const FRHISamplerStateDesc& InDesc)
        : Desc(InDesc)
    {
    }

    virtual ~FRHISamplerState() = default;

public:
    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }

    const FRHISamplerStateDesc& GetDesc() const { return Desc; }

protected:
    FRHISamplerStateDesc Desc;
};


struct FRHIViewportDesc
{
    FRHIViewportDesc()
        : WindowHandle(nullptr)
        , ColorFormat(EFormat::Unknown)
        , Width(0)
        , Height(0)
    {
    }

    FRHIViewportDesc(void* InWindowHandle, EFormat InColorFormat, uint16 InWidth, uint16 InHeight)
        : WindowHandle(InWindowHandle)
        , ColorFormat(InColorFormat)
        , Width(InWidth)
        , Height(InHeight)
    {
    }

    bool operator==(const FRHIViewportDesc& Other) const
    {
        return WindowHandle == Other.WindowHandle && ColorFormat == Other.ColorFormat && Width == Other.Width && Height == Other.Height;
    }

    bool operator!=(const FRHIViewportDesc& Other) const
    {
        return !(*this == Other);
    }

    void*   WindowHandle;
    EFormat ColorFormat;
    uint16  Width;
    uint16  Height;
};


class FRHIViewport : public FRHIResource
{
protected:
    explicit FRHIViewport(const FRHIViewportDesc& InDesc)
        : FRHIResource()
        , Desc(InDesc)
    {
    }

    virtual ~FRHIViewport() = default;

public:
    virtual bool Resize(uint32 InWidth, uint32 InHeight) { return true; }

    virtual FRHITexture* GetBackBuffer() const { return nullptr; };

    EFormat GetColorFormat() const
    {
        return Desc.ColorFormat;
    }
    
    uint32 GetWidth() const
    {
        return Desc.Width;
    }

    uint32 GetHeight() const
    {
        return Desc.Height;
    }

    const FRHIViewportDesc& GetDesc() const
    {
        return Desc;
    }

protected:
    FRHIViewportDesc Desc;
};


struct FRHITimestamp
{
    uint64 Begin = 0;
    uint64 End   = 0;
};


class FRHITimestampQuery : public FRHIResource
{
protected:
    FRHITimestampQuery()          = default;
    virtual ~FRHITimestampQuery() = default;

public:
    virtual void GetTimestampFromIndex(FRHITimestamp& OutQuery, uint32 Index) const = 0;

    virtual uint64 GetFrequency() const = 0;
};


enum class EStencilOp : uint8
{
    Keep    = 1,
    Zero    = 2,
    Replace = 3,
    IncrSat = 4,
    DecrSat = 5,
    Invert  = 6,
    Incr    = 7,
    Decr    = 8
};

constexpr const CHAR* ToString(EStencilOp StencilOp)
{
    switch (StencilOp)
    {
        case EStencilOp::Keep:    return "Keep";
        case EStencilOp::Zero:    return "Zero";
        case EStencilOp::Replace: return "Replace";
        case EStencilOp::IncrSat: return "IncrSat";
        case EStencilOp::DecrSat: return "DecrSat";
        case EStencilOp::Invert:  return "Invert";
        case EStencilOp::Incr:    return "Incr";
        case EStencilOp::Decr:    return "Decr";
        default:                  return "Unknown";
    }
}


struct FStencilState
{
    FStencilState()
        : StencilFailOp(EStencilOp::Keep)
        , StencilDepthFailOp(EStencilOp::Keep)
        , StencilDepthPassOp(EStencilOp::Keep)
        , StencilFunc(EComparisonFunc::Always)
    {
    }

    FStencilState(EStencilOp InStencilFailOp, EStencilOp InStencilDepthFailOp, EStencilOp InStencilPassOp, EComparisonFunc InStencilFunc)
        : StencilFailOp(InStencilFailOp)
        , StencilDepthFailOp(InStencilDepthFailOp)
        , StencilDepthPassOp(InStencilPassOp)
        , StencilFunc(InStencilFunc)
    {
    }

    uint64 GetHash() const
    {
        uint64 Hash = ToUnderlying(StencilFailOp);
        HashCombine(Hash, ToUnderlying(StencilDepthFailOp));
        HashCombine(Hash, ToUnderlying(StencilDepthPassOp));
        HashCombine(Hash, ToUnderlying(StencilFunc));
        return Hash;
    }

    bool operator==(const FStencilState& Other) const
    {
        return StencilFailOp      == Other.StencilFailOp 
            && StencilDepthFailOp == Other.StencilDepthFailOp
            && StencilDepthPassOp == Other.StencilDepthPassOp
            && StencilFunc        == Other.StencilFunc;
    }

    bool operator!=(const FStencilState& Other) const
    {
        return !(*this == Other);
    }

    EStencilOp      StencilFailOp;
    EStencilOp      StencilDepthFailOp;
    EStencilOp      StencilDepthPassOp;
    EComparisonFunc StencilFunc;
};


struct FRHIDepthStencilStateInitializer
{
    inline static constexpr uint32 DefaultMask = 0xffffffff;
    
    FRHIDepthStencilStateInitializer()
        : DepthFunc(EComparisonFunc::Less)
        , bDepthWriteEnable(true)
        , bDepthEnable(true)
        , StencilReadMask(DefaultMask)
        , StencilWriteMask(DefaultMask)
        , bStencilEnable(false)
        , FrontFace()
        , BackFace()
    {
    }

    FRHIDepthStencilStateInitializer(
        EComparisonFunc      InDepthFunc,
        bool                 bInDepthEnable,
        bool                 bInDepthWriteEnable = true,
        bool                 bInStencilEnable    = false,
        uint32               InStencilReadMask   = DefaultMask,
        uint32               InStencilWriteMask  = DefaultMask,
        const FStencilState& InFrontFace         = FStencilState(),
        const FStencilState& InBackFace          = FStencilState())
        : DepthFunc(InDepthFunc)
        , bDepthWriteEnable(bInDepthWriteEnable)
        , bDepthEnable(bInDepthEnable)
        , StencilReadMask(InStencilReadMask)
        , StencilWriteMask(InStencilWriteMask)
        , bStencilEnable(bInStencilEnable)
        , FrontFace(InFrontFace)
        , BackFace(InBackFace)
    {
    }

    uint64 GetHash() const
    {
        uint64 Hash = static_cast<uint64>(bDepthWriteEnable);
        HashCombine(Hash, ToUnderlying(DepthFunc));
        HashCombine(Hash, bDepthEnable);
        HashCombine(Hash, StencilReadMask);
        HashCombine(Hash, StencilWriteMask);
        HashCombine(Hash, bStencilEnable);
        HashCombine(Hash, FrontFace.GetHash());
        HashCombine(Hash, BackFace.GetHash());
        return Hash;
    }

    bool operator==(const FRHIDepthStencilStateInitializer& Other) const
    {
        return DepthFunc         == Other.DepthFunc
            && bDepthWriteEnable == Other.bDepthWriteEnable
            && bDepthEnable      == Other.bDepthEnable
            && StencilReadMask   == Other.StencilReadMask
            && StencilWriteMask  == Other.StencilWriteMask
            && bStencilEnable    == Other.bStencilEnable
            && FrontFace         == Other.FrontFace
            && BackFace          == Other.BackFace;
    }

    bool operator!=(const FRHIDepthStencilStateInitializer& Other) const
    {
        return !(*this == Other);
    }

    EComparisonFunc DepthFunc;
    bool            bDepthWriteEnable;
    bool            bDepthEnable;
    uint32          StencilReadMask;
    uint32          StencilWriteMask;
    bool            bStencilEnable;
    FStencilState   FrontFace;
    FStencilState   BackFace;
};


class FRHIDepthStencilState : public FRHIResource
{
protected:
    FRHIDepthStencilState()          = default;
    virtual ~FRHIDepthStencilState() = default;

public:
    virtual FRHIDepthStencilStateInitializer GetInitializer() const = 0; 
};


enum class ECullMode : uint8
{
    None  = 1,
    Front = 2,
    Back  = 3
};

constexpr const CHAR* ToString(ECullMode CullMode)
{
    switch (CullMode)
    {
        case ECullMode::None:  return "None";
        case ECullMode::Front: return "Front";
        case ECullMode::Back:  return "Back";
        default:               return "Unknown";
    }
}


enum class EFillMode : uint8
{
    WireFrame = 1,
    Solid     = 2
};

constexpr const CHAR* ToString(EFillMode FillMode)
{
    switch (FillMode)
    {
        case EFillMode::WireFrame: return "WireFrame";
        case EFillMode::Solid:     return "Solid";
        default:                   return "Unknown";
    }
}


struct FRHIRasterizerStateInitializer
{
    FRHIRasterizerStateInitializer()
        : FillMode(EFillMode::Solid)
        , CullMode(ECullMode::Back)
        , bFrontCounterClockwise(false)
        , bDepthClipEnable(true)
        , bMultisampleEnable(false)
        , bAntialiasedLineEnable(false)
        , bEnableConservativeRaster(false)
        , bEnableDepthBias(true)
        , ForcedSampleCount(0)
        , DepthBias(0.0f)
        , DepthBiasClamp(0.0f)
        , SlopeScaledDepthBias(0.0f)
    {
    }

    FRHIRasterizerStateInitializer(
        EFillMode InFillMode,
        ECullMode InCullMode,
        bool      bInFrontCounterClockwise    = false,
        float     InDepthBias                 = 0.0f,
        float     InDepthBiasClamp            = 0.0f,
        float     InSlopeScaledDepthBias      = 0.0f,
        bool      bInDepthClipEnable          = true,
        bool      bInMultisampleEnable        = false,
        bool      bInAntialiasedLineEnable    = false,
        uint32    InForcedSampleCount         = 1,
        bool      bInEnableConservativeRaster = false,
        bool      bInEnableDepthBias          = true)
        : FillMode(InFillMode)
        , CullMode(InCullMode)
        , bFrontCounterClockwise(bInFrontCounterClockwise)
        , bDepthClipEnable(bInDepthClipEnable)
        , bMultisampleEnable(bInMultisampleEnable)
        , bAntialiasedLineEnable(bInAntialiasedLineEnable)
        , bEnableConservativeRaster(bInEnableConservativeRaster)
        , bEnableDepthBias(bInEnableDepthBias)
        , ForcedSampleCount(InForcedSampleCount)
        , DepthBias(InDepthBias)
        , DepthBiasClamp(InDepthBiasClamp)
        , SlopeScaledDepthBias(InSlopeScaledDepthBias)
    {
    }

    uint64 GetHash() const
    {
        uint64 Hash = ToUnderlying(FillMode);
        HashCombine(Hash, ToUnderlying(CullMode));
        HashCombine(Hash, bFrontCounterClockwise);
        HashCombine(Hash, bDepthClipEnable);
        HashCombine(Hash, bMultisampleEnable);
        HashCombine(Hash, bAntialiasedLineEnable);
        HashCombine(Hash, bEnableConservativeRaster);
        HashCombine(Hash, ForcedSampleCount);
        HashCombine(Hash, DepthBias);
        HashCombine(Hash, DepthBiasClamp);
        HashCombine(Hash, SlopeScaledDepthBias);
        return Hash;
    }

    bool operator==(const FRHIRasterizerStateInitializer& Other) const
    {
        return FillMode                  == Other.FillMode
            && CullMode                  == Other.CullMode
            && bFrontCounterClockwise    == Other.bFrontCounterClockwise
            && DepthBias                 == Other.DepthBias
            && DepthBiasClamp            == Other.DepthBiasClamp
            && SlopeScaledDepthBias      == Other.SlopeScaledDepthBias
            && bDepthClipEnable          == Other.bDepthClipEnable
            && bMultisampleEnable        == Other.bMultisampleEnable
            && bAntialiasedLineEnable    == Other.bAntialiasedLineEnable
            && ForcedSampleCount         == Other.ForcedSampleCount
            && bEnableConservativeRaster == Other.bEnableConservativeRaster;
    }

    bool operator!=(const FRHIRasterizerStateInitializer& Other) const
    {
        return !(*this == Other);
    }

    EFillMode FillMode;
    ECullMode CullMode;
    bool      bFrontCounterClockwise;
    bool      bDepthClipEnable;
    bool      bMultisampleEnable;
    bool      bAntialiasedLineEnable;
    bool      bEnableConservativeRaster;
    bool      bEnableDepthBias;
    uint32    ForcedSampleCount;
    float     DepthBias;
    float     DepthBiasClamp;
    float     SlopeScaledDepthBias;
};


class FRHIRasterizerState : public FRHIResource
{
protected:
    FRHIRasterizerState()          = default;
    virtual ~FRHIRasterizerState() = default;

public:
    virtual FRHIRasterizerStateInitializer GetInitializer() const = 0; 
};


enum class EBlendType : uint8
{
    Zero           = 1,
    One            = 2,
    SrcColor       = 3,
    InvSrcColor    = 4,
    SrcAlpha       = 5,
    InvSrcAlpha    = 6,
    DstAlpha       = 7,
    InvDstAlpha    = 8,
    DstColor       = 9,
    InvDstColor    = 10,
    SrcAlphaSat    = 11,
    BlendFactor    = 12,
    InvBlendFactor = 13,
    Src1Color      = 14,
    InvSrc1Color   = 15,
    Src1Alpha      = 16,
    InvSrc1Alpha   = 17
};

constexpr const CHAR* ToString(EBlendType  Blend)
{
    switch (Blend)
    {
        case EBlendType ::Zero:           return "Zero";
        case EBlendType ::One:            return "One";
        case EBlendType ::SrcColor:       return "SrcColor";
        case EBlendType ::InvSrcColor:    return "InvSrcColor";
        case EBlendType ::SrcAlpha:       return "SrcAlpha";
        case EBlendType ::InvSrcAlpha:    return "InvSrcAlpha";
        case EBlendType ::DstAlpha:       return "DstAlpha";
        case EBlendType ::InvDstAlpha:    return "InvDstAlpha";
        case EBlendType ::DstColor:       return "DstColor";
        case EBlendType ::InvDstColor:    return "InvDstColor";
        case EBlendType ::SrcAlphaSat:    return "SrcAlphaSat";
        case EBlendType ::BlendFactor:    return "BlendFactor";
        case EBlendType ::InvBlendFactor: return "InvBlendFactor";
        case EBlendType ::Src1Color:      return "Src1Color";
        case EBlendType ::InvSrc1Color:   return "InvSrc1Color";
        case EBlendType ::Src1Alpha:      return "Src1Alpha";
        case EBlendType ::InvSrc1Alpha:   return "InvSrc1Alpha";
        default:                          return "Unknown";
    }
}


enum class EBlendOp : uint8
{
    Add         = 1,
    Subtract    = 2,
    RevSubtract = 3,
    Min         = 4,
    Max         = 5
};

constexpr const CHAR* ToString(EBlendOp BlendOp)
{
    switch (BlendOp)
    {
        case EBlendOp::Add:         return "Add";
        case EBlendOp::Subtract:    return "Subtract";
        case EBlendOp::RevSubtract: return "RevSubtract";
        case EBlendOp::Min:         return "Min";
        case EBlendOp::Max:         return "Max";
        default:                    return "Unknown";
    }
}


enum class ELogicOp : uint8
{
    Clear        = 0,
    Set          = 1,
    Copy         = 2,
    CopyInverted = 3,
    NoOp         = 4,
    Invert       = 5,
    And          = 6,
    Nand         = 7,
    Or           = 8,
    Nor          = 9,
    Xor          = 10,
    Equivalent   = 11,
    AndReverse   = 12,
    AndInverted  = 13,
    OrReverse    = 14,
    OrInverted   = 15
};

constexpr const CHAR* ToString(ELogicOp LogicOp)
{
    switch (LogicOp)
    {
        case ELogicOp::Clear:        return "Clear";
        case ELogicOp::Set:          return "Set";
        case ELogicOp::Copy:         return "Copy";
        case ELogicOp::CopyInverted: return "CopyInverted";
        case ELogicOp::NoOp:         return "NoOp";
        case ELogicOp::Invert:       return "Invert";
        case ELogicOp::And:          return "And";
        case ELogicOp::Nand:         return "Nand";
        case ELogicOp::Or:           return "Or";
        case ELogicOp::Nor:          return "Nor";
        case ELogicOp::Xor:          return "Xor";
        case ELogicOp::Equivalent:   return "Equivalent";
        case ELogicOp::AndReverse:   return "AndReverse";
        case ELogicOp::AndInverted:  return "AndInverted";
        case ELogicOp::OrReverse:    return "OrReverse";
        case ELogicOp::OrInverted:   return "OrInverted";
        default:                     return "Unknown";
    }
}


enum class EColorWriteFlags : uint8
{
    None  = 0,
    Red   = FLAG(0),
    Green = FLAG(1),
    Blue  = FLAG(2),
    Alpha = FLAG(3),
    All   = Red | Green | Blue | Alpha
};

ENUM_CLASS_OPERATORS(EColorWriteFlags);


struct FRenderTargetBlendDesc
{
    FRenderTargetBlendDesc()
        : SrcBlend(EBlendType::One)
        , DstBlend(EBlendType::Zero)
        , BlendOp(EBlendOp::Add)
        , SrcBlendAlpha(EBlendType::One)
        , DstBlendAlpha(EBlendType::Zero)
        , BlendOpAlpha(EBlendOp::Add)
        , bBlendEnable(false)
        , ColorWriteMask(EColorWriteFlags::All)
    {
    }

    FRenderTargetBlendDesc(
        bool             bInBlendEnable,
        EBlendType       InSrcBlend,
        EBlendType       InDstBlend,
        EBlendOp         InBlendOp        = EBlendOp::Add,
        EBlendType       InSrcBlendAlpha  = EBlendType::One,
        EBlendType       InDstBlendAlpha  = EBlendType::Zero,
        EBlendOp         InBlendOpAlpha   = EBlendOp::Add,
        EColorWriteFlags InColorWriteMask = EColorWriteFlags::All)
        : SrcBlend(InSrcBlend)
        , DstBlend(InDstBlend)
        , BlendOp(InBlendOp)
        , SrcBlendAlpha(InSrcBlendAlpha)
        , DstBlendAlpha(InDstBlendAlpha)
        , BlendOpAlpha(InBlendOpAlpha)
        , bBlendEnable(bInBlendEnable)
        , ColorWriteMask(InColorWriteMask)
    {
    }

    uint64 GetHash() const
    {
        uint64 Hash = ToUnderlying(SrcBlend);
        HashCombine(Hash, ToUnderlying(DstBlend));
        HashCombine(Hash, ToUnderlying(BlendOp));
        HashCombine(Hash, ToUnderlying(SrcBlendAlpha));
        HashCombine(Hash, ToUnderlying(DstBlendAlpha));
        HashCombine(Hash, ToUnderlying(BlendOpAlpha));
        HashCombine(Hash, bBlendEnable);
        HashCombine(Hash, ToUnderlying(ColorWriteMask));
        return Hash;
    }

    bool operator==(const FRenderTargetBlendDesc& Other) const
    {
        return SrcBlend       == Other.SrcBlend
            && DstBlend       == Other.DstBlend
            && BlendOp        == Other.BlendOp
            && SrcBlendAlpha  == Other.SrcBlendAlpha
            && DstBlendAlpha  == Other.DstBlendAlpha
            && BlendOpAlpha   == Other.BlendOpAlpha
            && bBlendEnable   == Other.bBlendEnable
            && ColorWriteMask == Other.ColorWriteMask;
    }

    bool operator!=(const FRenderTargetBlendDesc& Other) const
    {
        return !(*this == Other);
    }

    EBlendType       SrcBlend;
    EBlendType       DstBlend;
    EBlendOp         BlendOp;
    EBlendType       SrcBlendAlpha;
    EBlendType       DstBlendAlpha;
    EBlendOp         BlendOpAlpha;
    bool             bBlendEnable;
    EColorWriteFlags ColorWriteMask;
};


struct FRHIBlendStateInitializer
{
    FRHIBlendStateInitializer()
        : RenderTargets()
        , NumRenderTargets(0)
        , LogicOp(ELogicOp::NoOp)
        , bLogicOpEnable(false)
        , bAlphaToCoverageEnable(false)
        , bIndependentBlendEnable(false)
    {
    }

    uint64 GetHash() const
    {
        uint64 Hash = 0;
        for (uint32 Index = 0; Index < NumRenderTargets; ++Index)
        {
            HashCombine(Hash, RenderTargets[Index].GetHash());
        }

        HashCombine(Hash, ToUnderlying(LogicOp));
        HashCombine(Hash, bLogicOpEnable);
        HashCombine(Hash, bAlphaToCoverageEnable);
        HashCombine(Hash, bIndependentBlendEnable);
        return Hash;
    }

    bool operator==(const FRHIBlendStateInitializer& Other) const
    {
        return NumRenderTargets == Other.NumRenderTargets
            && CompareArrays(RenderTargets, Other.RenderTargets, NumRenderTargets)
            && LogicOp                 == Other.LogicOp
            && bLogicOpEnable          == Other.bLogicOpEnable
            && bAlphaToCoverageEnable  == Other.bAlphaToCoverageEnable 
            && bIndependentBlendEnable == Other.bIndependentBlendEnable;
    }

    bool operator!=(const FRHIBlendStateInitializer& Other) const
    {
        return !(*this == Other);
    }

    FRenderTargetBlendDesc RenderTargets[FRHILimits::MaxRenderTargets];
    uint8                  NumRenderTargets;
    ELogicOp               LogicOp;

    bool bLogicOpEnable          : 1;
    bool bAlphaToCoverageEnable  : 1;
    bool bIndependentBlendEnable : 1;
};


class FRHIBlendState : public FRHIResource
{
protected:
    FRHIBlendState()          = default;
    virtual ~FRHIBlendState() = default;

public:
    virtual FRHIBlendStateInitializer GetInitializer() const = 0;
};


enum class EVertexInputClass : uint8
{
    Vertex   = 0,
    Instance = 1,
};

constexpr const CHAR* ToString(EVertexInputClass BlendOp)
{
    switch (BlendOp)
    {
    case EVertexInputClass::Vertex:   return "Vertex";
    case EVertexInputClass::Instance: return "Instance";
    default:                          return "Unknown";
    }
}


struct FVertexInputElement
{
    FVertexInputElement()
        : Semantic("")
        , SemanticIndex(0)
        , Format(EFormat::Unknown)
        , VertexStride(0)
        , InputSlot(0)
        , ByteOffset(0)
        , InputClass(EVertexInputClass::Vertex)
        , InstanceStepRate(0)
    {
    }

    FVertexInputElement(
        const FString&    InSemantic,
        uint32            InSemanticIndex,
        EFormat           InFormat,
        uint16            InVertexStride,
        uint32            InInputSlot,
        uint32            InByteOffset,
        EVertexInputClass InInputClass,
        uint32            InInstanceStepRate)
        : Semantic(InSemantic)
        , SemanticIndex(InSemanticIndex)
        , Format(InFormat)
        , VertexStride(InVertexStride)
        , InputSlot(InInputSlot)
        , ByteOffset(InByteOffset)
        , InputClass(InInputClass)
        , InstanceStepRate(InInstanceStepRate)
    {
    }

    bool operator==(const FVertexInputElement& Other) const
    {
        return Semantic         == Other.Semantic
            && SemanticIndex    == Other.SemanticIndex
            && Format           == Other.Format
            && VertexStride     == Other.VertexStride
            && InputSlot        == Other.InputSlot
            && ByteOffset       == Other.ByteOffset
            && InputClass       == Other.InputClass
            && InstanceStepRate == Other.InstanceStepRate;
    }

    bool operator!=(const FVertexInputElement& Other) const
    {
        return !(*this == Other);
    }

    FString           Semantic;
    uint32            SemanticIndex;
    EFormat           Format;
    uint16            VertexStride;
    uint32            InputSlot;
    uint32            ByteOffset;
    EVertexInputClass InputClass;
    uint32            InstanceStepRate;
};


struct FRHIVertexInputLayoutInitializer
{
    FRHIVertexInputLayoutInitializer() = default;

    FRHIVertexInputLayoutInitializer(const TArray<FVertexInputElement>& InElements)
        : Elements(InElements)
    {
    }

    FRHIVertexInputLayoutInitializer(std::initializer_list<FVertexInputElement> InList)
        : Elements(InList)
    {
    }

    bool operator==(const FRHIVertexInputLayoutInitializer& Other) const
    {
        return Elements == Other.Elements;
    }

    bool operator!=(const FRHIVertexInputLayoutInitializer& Other) const
    {
        return !(*this == Other);
    }

    TArray<FVertexInputElement> Elements;
};


class FRHIVertexInputLayout : public FRHIResource
{
protected:
    FRHIVertexInputLayout()          = default;
    virtual ~FRHIVertexInputLayout() = default;
};


class FRHIPipelineState : public FRHIResource
{
protected:
    FRHIPipelineState()          = default;
    virtual ~FRHIPipelineState() = default;

public:
    virtual void SetName(const FString& InName) { }

    virtual FString GetName() const { return ""; }
};


struct FGraphicsPipelineFormats
{
    FGraphicsPipelineFormats()
        : RenderTargetFormats()
        , NumRenderTargets(0)
        , DepthStencilFormat(EFormat::Unknown)
    {
    }

    bool operator==(const FGraphicsPipelineFormats& Other) const
    {
        for (uint32 Index = 0; Index < NumRenderTargets; Index++)
        {
            if (RenderTargetFormats[Index] == Other.RenderTargetFormats[Index])
            {
                return false;
            }
        }

        return NumRenderTargets == Other.NumRenderTargets && DepthStencilFormat == Other.DepthStencilFormat;
    }

    bool operator!=(const FGraphicsPipelineFormats& Other) const
    {
        return !(*this == Other);
    }

    EFormat RenderTargetFormats[FRHILimits::MaxRenderTargets];
    uint32  NumRenderTargets;
    EFormat DepthStencilFormat;
};


struct FGraphicsPipelineShaders
{
    FGraphicsPipelineShaders()
        : VertexShader(nullptr)
        , HullShader(nullptr)
        , DomainShader(nullptr)
        , GeometryShader(nullptr)
        , PixelShader(nullptr)
    {
    }

    FGraphicsPipelineShaders(
        FRHIVertexShader*   InVertexShader,
        FRHIHullShader*     InHullShader,
        FRHIDomainShader*   InDomainShader,
        FRHIGeometryShader* InGeometryShader,
        FRHIPixelShader*    InPixelShader)
        : VertexShader(InVertexShader)
        , HullShader(InHullShader)
        , DomainShader(InDomainShader)
        , GeometryShader(InGeometryShader)
        , PixelShader(InPixelShader)
    {
    }

    bool operator==(const FGraphicsPipelineShaders& Other) const
    {
        return VertexShader   == Other.VertexShader 
            && HullShader     == Other.HullShader
            && DomainShader   == Other.DomainShader
            && GeometryShader == Other.GeometryShader
            && PixelShader    == Other.PixelShader;
    }

    bool operator!=(const FGraphicsPipelineShaders& Other) const
    {
        return !(*this == Other);
    }

    FRHIVertexShader*   VertexShader;
    FRHIHullShader*     HullShader;
    FRHIDomainShader*   DomainShader;
    FRHIGeometryShader* GeometryShader;
    FRHIPixelShader*    PixelShader;
};


struct FRHIGraphicsPipelineStateInitializer
{
    FRHIGraphicsPipelineStateInitializer()
        : VertexInputLayout(nullptr)
        , DepthStencilState(nullptr)
        , RasterizerState(nullptr)
        , BlendState(nullptr)
        , SampleCount(1)
        , SampleQuality(0)
        , SampleMask(0xffffffff)
        , bPrimitiveRestartEnable(false)
        , PrimitiveTopology(EPrimitiveTopology::TriangleList)
        , ShaderState()
        , PipelineFormats()
    {
    }

    FRHIGraphicsPipelineStateInitializer(
        FRHIVertexInputLayout*          InVertexInputLayout,
        FRHIDepthStencilState*          InDepthStencilState,
        FRHIRasterizerState*            InRasterizerState,
        FRHIBlendState*                 InBlendState,
        const FGraphicsPipelineShaders& InShaderState,
        const FGraphicsPipelineFormats& InPipelineFormats,
        EPrimitiveTopology              InPrimitiveTopology       = EPrimitiveTopology::TriangleList,
        uint32                          InSampleCount             = 1,
        uint32                          InSampleQuality           = 0,
        uint32                          InSampleMask              = 0xffffffff,
        bool                            bInPrimitiveRestartEnable = false)
        : VertexInputLayout(InVertexInputLayout)
        , DepthStencilState(InDepthStencilState)
        , RasterizerState(InRasterizerState)
        , BlendState(InBlendState)
        , SampleCount(InSampleCount)
        , SampleQuality(InSampleQuality)
        , SampleMask(InSampleMask)
        , bPrimitiveRestartEnable(bInPrimitiveRestartEnable)
        , PrimitiveTopology(InPrimitiveTopology)
        , ShaderState(InShaderState)
        , PipelineFormats(InPipelineFormats)
    {
    }

    bool operator==(const FRHIGraphicsPipelineStateInitializer& Other) const
    {
        return VertexInputLayout       == Other.VertexInputLayout
            && DepthStencilState       == Other.DepthStencilState
            && RasterizerState         == Other.RasterizerState
            && BlendState              == Other.BlendState
            && SampleCount             == Other.SampleCount
            && SampleQuality           == Other.SampleQuality
            && SampleMask              == Other.SampleMask
            && bPrimitiveRestartEnable == Other.bPrimitiveRestartEnable
            && PrimitiveTopology       == Other.PrimitiveTopology
            && ShaderState             == Other.ShaderState
            && PipelineFormats         == Other.PipelineFormats;
    }

    bool operator!=(const FRHIGraphicsPipelineStateInitializer& Other) const
    {
        return !(*this == Other);
    }

    // Weak reference to the VertexInputLayout being used
    FRHIVertexInputLayout* VertexInputLayout;

    // Weak reference to the DepthStencilState being used
    FRHIDepthStencilState* DepthStencilState;

    // Weak reference to the RasterizerState being used
    FRHIRasterizerState* RasterizerState;

    // Weak reference to the BlendState being used
    FRHIBlendState* BlendState;

    uint32 SampleCount;
    uint32 SampleQuality;
    uint32 SampleMask;

    EPrimitiveTopology PrimitiveTopology;
    bool               bPrimitiveRestartEnable;

    FGraphicsPipelineShaders ShaderState;
    FGraphicsPipelineFormats PipelineFormats;
};


class FRHIGraphicsPipelineState : public FRHIPipelineState
{
protected:
    FRHIGraphicsPipelineState()  = default;
    ~FRHIGraphicsPipelineState() = default;
};


struct FRHIComputePipelineStateInitializer
{
    FRHIComputePipelineStateInitializer()
        : Shader(nullptr)
    {
    }

    FRHIComputePipelineStateInitializer(FRHIComputeShader* InShader)
        : Shader(InShader)
    {
    }

    bool operator==(const FRHIComputePipelineStateInitializer& Other) const
    {
        return Shader == Other.Shader;
    }

    bool operator!=(const FRHIComputePipelineStateInitializer& Other) const
    {
        return !(*this == Other);
    }

    FRHIComputeShader* Shader;
};


class FRHIComputePipelineState : public FRHIPipelineState
{
protected:
    FRHIComputePipelineState()          = default;
    virtual ~FRHIComputePipelineState() = default;
};


enum class ERayTracingHitGroupType : uint8
{
    Unknown    = 0,
    Triangles  = 1,
    Procedural = 2
};


struct FRHIRayTracingHitGroupDesc
{
    FRHIRayTracingHitGroupDesc() = default;

    FRHIRayTracingHitGroupDesc(const FString& InName, ERayTracingHitGroupType InType, TArrayView<FRHIRayTracingShader*> InRayTracingShaders)
        : Name(InName)
        , Type(InType)
        , Shaders(InRayTracingShaders)
    {
    }

    bool operator==(const FRHIRayTracingHitGroupDesc& Other) const
    {
        return Name == Other.Name && Shaders == Other.Shaders && Type == Other.Type;
    }

    bool operator!=(const FRHIRayTracingHitGroupDesc& Other) const
    {
        return !(*this == Other);
    }

    FString                       Name;
    ERayTracingHitGroupType       Type{ ERayTracingHitGroupType::Unknown };
    TArray<FRHIRayTracingShader*> Shaders;
};


struct FRHIRayTracingPipelineStateDesc
{
    FRHIRayTracingPipelineStateDesc() = default;

    FRHIRayTracingPipelineStateDesc(
        const TArrayView<FRHIRayGenShader*>&          InRayGenShaders,
        const TArrayView<FRHIRayCallableShader*>&     InCallableShaders,
        const TArrayView<FRHIRayTracingHitGroupDesc>& InHitGroups,
        const TArrayView<FRHIRayMissShader*>&         InMissShaders,
        uint32 InMaxAttributeSizeInBytes,
        uint32 InMaxPayloadSizeInBytes,
        uint32 InMaxRecursionDepth)
        : RayGenShaders(InRayGenShaders)
        , CallableShaders(InCallableShaders)
        , MissShaders(InMissShaders)
        , HitGroups(InHitGroups)
        , MaxAttributeSizeInBytes(InMaxAttributeSizeInBytes)
        , MaxPayloadSizeInBytes(InMaxPayloadSizeInBytes)
        , MaxRecursionDepth(InMaxRecursionDepth)
    {
    }

    bool operator==(const FRHIRayTracingPipelineStateDesc& Other) const
    {
        return RayGenShaders           == Other.RayGenShaders
            && CallableShaders         == Other.CallableShaders
            && HitGroups               == Other.HitGroups
            && MissShaders             == Other.MissShaders
            && MaxAttributeSizeInBytes == Other.MaxAttributeSizeInBytes
            && MaxPayloadSizeInBytes   == Other.MaxPayloadSizeInBytes
            && MaxRecursionDepth       == Other.MaxRecursionDepth;
    }

    bool operator!=(const FRHIRayTracingPipelineStateDesc& Other) const
    {
        return !(*this == Other);
    }

    TArray<FRHIRayGenShader*>          RayGenShaders;
    TArray<FRHIRayCallableShader*>     CallableShaders;
    TArray<FRHIRayMissShader*>         MissShaders;
    TArray<FRHIRayTracingHitGroupDesc> HitGroups;
    
    uint32 MaxAttributeSizeInBytes{0};
    uint32 MaxPayloadSizeInBytes{0};
    uint32 MaxRecursionDepth{1};
};


class FRHIRayTracingPipelineState : public FRHIPipelineState
{
protected:
    FRHIRayTracingPipelineState()          = default;
    virtual ~FRHIRayTracingPipelineState() = default;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
