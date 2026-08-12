#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/StaticArray.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIResourceViews.h"
#include "RendererCore/RenderGraph/RenderGraphResources.h"

class FRenderGraphPass;

enum class ERenderGraphPassFlags : uint8
{
    None = 0,

    /** The pass draws, so the graph opens a render pass around its execute lambda */
    Raster = FLAG(0),

    /** The pass dispatches compute work */
    Compute = FLAG(1),

    /** The pass only copies */
    Copy = FLAG(2),

    /** The pass survives compilation even when nothing reads what it writes */
    NeverCull = FLAG(3),
};

ENUM_CLASS_OPERATORS(ERenderGraphPassFlags);

template<typename ResourceType>
struct TRenderGraphResourceAccess
{
    ResourceType*     Resource = nullptr;
    ERHIResourceState State    = ERHIResourceState::Common;
    bool              bIsWrite = false;
};

typedef TRenderGraphResourceAccess<FRenderGraphTexture> FRenderGraphTextureAccess;
typedef TRenderGraphResourceAccess<FRenderGraphBuffer>  FRenderGraphBufferAccess;

struct FRenderGraphAttachment
{
    FRenderGraphTexture*   Texture                = nullptr;
    FFloatColor            ClearValue             = FFloatColor(0.0f, 0.0f, 0.0f, 1.0f);
    FDepthStencilValue     DepthStencilClearValue = FDepthStencilValue(1.0f, 0);
    EAttachmentLoadAction  LoadAction             = EAttachmentLoadAction::DontCare;
    EAttachmentStoreAction StoreAction            = EAttachmentStoreAction::DontCare;
    bool                   bReadOnly              = false;
};

class RENDERERCORE_API FRenderGraphPassResources
{
    friend class FRenderGraphBuilder;

    explicit FRenderGraphPassResources(const FRenderGraphPass& InPass)
        : Pass(InPass)
    {
    }

public:
    NODISCARD FRHIBuffer*  Get(FRenderGraphBuffer* Buffer)   const;
    NODISCARD FRHITexture* Get(FRenderGraphTexture* Texture) const;

private:
    const FRenderGraphPass& Pass;
};

struct FRenderGraphPassExecutor
{
    virtual ~FRenderGraphPassExecutor() = default;

    virtual void Execute(FRHICommandList& CommandList, const FRenderGraphPassResources& Resources) = 0;
};

template<typename LambdaType>
struct TRenderGraphPassExecutor final : public FRenderGraphPassExecutor
{
    explicit TRenderGraphPassExecutor(LambdaType&& InLambda)
        : Lambda(Forward<LambdaType>(InLambda))
    {
    }

    virtual void Execute(FRHICommandList& CommandList, const FRenderGraphPassResources& Resources) override final
    {
        Lambda(CommandList, Resources);
    }

    LambdaType Lambda;
};

class RENDERERCORE_API FRenderGraphPass
{
    friend class FRenderGraphBuilder;
    friend class FRenderGraphPassBuilder;
    friend class FRenderGraphPassResources;

public:
    FRenderGraphPass(const CHAR* InName, ERenderGraphPassFlags InFlags)
        : Name(InName ? InName : "RenderGraphPass")
        , Flags(InFlags)
        , Executor(nullptr)
        , TextureAccesses()
        , BufferAccesses()
        , RenderTargets()
        , DepthStencil()
        , NumRenderTargets(0)
        , Transitions()
        , UnorderedAccessBarriers()
        , bIsCulled(false)
    {
    }

    NODISCARD const CHAR* GetName() const
    {
        return Name;
    }

    NODISCARD ERenderGraphPassFlags GetFlags() const
    {
        return Flags;
    }

    NODISCARD bool IsCulled() const
    {
        return bIsCulled;
    }

    NODISCARD bool IsRaster() const
    {
        return IsEnumFlagSet(Flags, ERenderGraphPassFlags::Raster);
    }

private:
    const CHAR*                                                  Name;
    ERenderGraphPassFlags                                        Flags;
    FRenderGraphPassExecutor*                                    Executor;
    TArray<FRenderGraphTextureAccess>                            TextureAccesses;
    TArray<FRenderGraphBufferAccess>                             BufferAccesses;
    TStaticArray<FRenderGraphAttachment, RHI_MAX_RENDER_TARGETS> RenderTargets;
    FRenderGraphAttachment                                       DepthStencil;
    uint32                                                       NumRenderTargets;
    TArray<FRHITransitionBarrierDesc>                            Transitions;
    TArray<FRHIUnorderedAccessBarrierDesc>                       UnorderedAccessBarriers;
    bool                                                         bIsCulled;
};

class RENDERERCORE_API FRenderGraphPassBuilder
{
    friend class FRenderGraphBuilder;

    explicit FRenderGraphPassBuilder(FRenderGraphPass& InPass)
        : Pass(InPass)
    {
    }

public:
    void ReadTexture(FRenderGraphTexture* Texture, ERHIResourceState State);
    void WriteTexture(FRenderGraphTexture* Texture, ERHIResourceState State);

    void ReadBuffer(FRenderGraphBuffer* Buffer, ERHIResourceState State);
    void WriteBuffer(FRenderGraphBuffer* Buffer, ERHIResourceState State);

    void SetRenderTarget(uint32 Index, FRenderGraphTexture* Texture, EAttachmentLoadAction LoadAction = EAttachmentLoadAction::Clear,
        EAttachmentStoreAction StoreAction = EAttachmentStoreAction::Store, const FFloatColor& ClearValue = FFloatColor(0.0f, 0.0f, 0.0f, 1.0f));

    void SetDepthStencil(FRenderGraphTexture* Texture, EAttachmentLoadAction LoadAction = EAttachmentLoadAction::Clear,
        EAttachmentStoreAction StoreAction = EAttachmentStoreAction::Store, const FDepthStencilValue& ClearValue = FDepthStencilValue(1.0f, 0), bool bReadOnly = false);

private:
    FRenderGraphPass& Pass;
};
