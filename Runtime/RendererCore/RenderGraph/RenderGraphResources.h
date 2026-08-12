#pragma once
#include "RHI/RHIBuffer.h"
#include "RHI/RHITexture.h"

class FRenderGraphBuilder;
class FRenderGraphPassBuilder;

struct FRenderGraphTextureDesc
{
    NODISCARD static FRenderGraphTextureDesc CreateTexture2D(
        EFormat            InFormat,
        uint32             InWidth,
        uint32             InHeight,
        uint32             InNumMipLevels,
        uint32             InNumSamples,
        ETextureUsageFlags InUsageFlags,
        const FClearValue& InClearValue = FClearValue())
    {
        return FRenderGraphTextureDesc(FRHITextureDesc::CreateTexture2D(InFormat, InWidth, InHeight, InNumMipLevels, InNumSamples, InUsageFlags, InClearValue));
    }

    NODISCARD static FRenderGraphTextureDesc CreateTexture2DArray(
        EFormat            InFormat,
        uint32             InWidth,
        uint32             InHeight,
        uint32             InArraySlices,
        uint32             InNumMipLevels,
        uint32             InNumSamples,
        ETextureUsageFlags InUsageFlags,
        const FClearValue& InClearValue = FClearValue())
    {
        return FRenderGraphTextureDesc(FRHITextureDesc::CreateTexture2DArray(InFormat, InWidth, InHeight, InArraySlices, InNumMipLevels, InNumSamples, InUsageFlags, InClearValue));
    }

    NODISCARD static FRenderGraphTextureDesc CreateTextureCube(
        EFormat            InFormat,
        uint32             InExtent,
        uint32             InNumMipLevels,
        uint32             InNumSamples,
        ETextureUsageFlags InUsageFlags,
        const FClearValue& InClearValue = FClearValue())
    {
        return FRenderGraphTextureDesc(FRHITextureDesc::CreateTextureCube(InFormat, InExtent, InNumMipLevels, InNumSamples, InUsageFlags, InClearValue));
    }

    NODISCARD static FRenderGraphTextureDesc CreateTexture3D(
        EFormat            InFormat,
        uint32             InWidth,
        uint32             InHeight,
        uint32             InDepth,
        uint32             InNumMipLevels,
        uint32             InNumSamples,
        ETextureUsageFlags InUsageFlags,
        const FClearValue& InClearValue = FClearValue())
    {
        return FRenderGraphTextureDesc(FRHITextureDesc::CreateTexture3D(InFormat, InWidth, InHeight, InDepth, InNumMipLevels, InNumSamples, InUsageFlags, InClearValue));
    }

    FRenderGraphTextureDesc() noexcept = default;

    explicit FRenderGraphTextureDesc(const FRHITextureDesc& InTextureDesc) noexcept
        : TextureDesc(InTextureDesc)
    {
    }

    NODISCARD bool operator==(const FRenderGraphTextureDesc& Other) const noexcept = default;

    FRHITextureDesc TextureDesc = { };
};

struct FRenderGraphBufferDesc
{
    NODISCARD static FRenderGraphBufferDesc CreateVertexBuffer(uint32 InStride, uint32 InNumElements, EBufferFlags InExtraFlags = EBufferFlags::None)
    {
        return FRenderGraphBufferDesc(FRHIBufferDesc::CreateVertexBuffer(InStride, InNumElements, EBufferFlags::Default | InExtraFlags));
    }

    NODISCARD static FRenderGraphBufferDesc CreateIndexBuffer(uint32 InStride, uint32 InNumElements, EBufferFlags InExtraFlags = EBufferFlags::None)
    {
        return FRenderGraphBufferDesc(FRHIBufferDesc::CreateIndexBuffer(InStride, InNumElements, EBufferFlags::Default | InExtraFlags));
    }

    NODISCARD static FRenderGraphBufferDesc CreateStructuredBuffer(uint32 InStride, uint32 InNumElements, EBufferFlags InExtraFlags = EBufferFlags::None)
    {
        return FRenderGraphBufferDesc(FRHIBufferDesc::CreateStructuredBuffer(InStride, InNumElements, EBufferFlags::Default | InExtraFlags));
    }

    NODISCARD static FRenderGraphBufferDesc CreateConstantBuffer(uint64 InSize, EBufferFlags InExtraFlags = EBufferFlags::None)
    {
        return FRenderGraphBufferDesc(FRHIBufferDesc::CreateConstantBuffer(InSize, EBufferFlags::Default | InExtraFlags));
    }

    FRenderGraphBufferDesc() noexcept = default;

    FRenderGraphBufferDesc(EBufferFlags InFlags, uint32 InStride, uint64 InSize) noexcept
        : BufferDesc(InFlags, InStride, InSize)
    {
    }

    explicit FRenderGraphBufferDesc(const FRHIBufferDesc& InBufferDesc) noexcept
        : BufferDesc(InBufferDesc)
    {
    }

    NODISCARD bool operator==(const FRenderGraphBufferDesc& Other) const noexcept
    {
        return BufferDesc.Flags  == Other.BufferDesc.Flags
            && BufferDesc.Stride == Other.BufferDesc.Stride
            && BufferDesc.Size   == Other.BufferDesc.Size
            && BufferDesc.TrackingMode == Other.BufferDesc.TrackingMode;
    }

    FRHIBufferDesc BufferDesc = { };
};

struct FRenderGraphResourceState
{
    /** Number of surviving passes that read the resource, which drives pass culling */
    int32             NumReaders = 0;

    /** Index of the first and last surviving pass to touch the resource, which bounds its lifetime. -1 when untouched */
    int32             FirstPassIndex = -1;
    int32             LastPassIndex  = -1;

    /** State the resource is in at the point the barrier walk has reached */
    ERHIResourceState CurrentState = ERHIResourceState::Common;

    /** State the graph must leave the resource in, only meaningful for an external resource */
    ERHIResourceState FinalState = ERHIResourceState::Common;

    /** Set by an UnorderedAccess write, so a following UnorderedAccess access knows to fence */
    bool              bWrittenAsUnorderedAccess = false;
};

class RENDERERCORE_API FRenderGraphTexture
{
    friend class FRenderGraphBuilder;
    friend class FRenderGraphPassBuilder;

public:
    FRenderGraphTexture(const FRenderGraphTextureDesc& InDesc, const CHAR* InName, FRHITexture* InExternalTexture)
        : Desc(InDesc)
        , Name(InName)
        , Texture(InExternalTexture)
        , bIsExternal(InExternalTexture != nullptr)
        , State()
    {
    }

    NODISCARD const FRenderGraphTextureDesc& GetDesc() const
    {
        return Desc;
    }

    NODISCARD const CHAR* GetName() const
    {
        return Name;
    }

    NODISCARD bool IsExternal() const
    {
        return bIsExternal;
    }

    NODISCARD FRHITexture* GetRHITexture() const
    {
        return Texture;
    }

private:
    FRenderGraphTextureDesc   Desc;
    const CHAR*               Name;
    FRHITexture*              Texture;
    bool                      bIsExternal;
    FRenderGraphResourceState State;
};

class RENDERERCORE_API FRenderGraphBuffer
{
    friend class FRenderGraphBuilder;
    friend class FRenderGraphPassBuilder;

public:
    FRenderGraphBuffer(const FRenderGraphBufferDesc& InDesc, const CHAR* InName, FRHIBuffer* InExternalBuffer)
        : Desc(InDesc)
        , Name(InName)
        , Buffer(InExternalBuffer)
        , bIsExternal(InExternalBuffer != nullptr)
        , State()
    {
    }

    NODISCARD const FRenderGraphBufferDesc& GetDesc() const
    {
        return Desc;
    }

    NODISCARD const CHAR* GetName() const
    {
        return Name;
    }

    NODISCARD bool IsExternal() const
    {
        return bIsExternal;
    }

    NODISCARD FRHIBuffer* GetRHIBuffer() const
    {
        return Buffer;
    }

private:
    FRenderGraphBufferDesc    Desc;
    const CHAR*               Name;
    FRHIBuffer*               Buffer;
    bool                      bIsExternal;
    FRenderGraphResourceState State;
};
