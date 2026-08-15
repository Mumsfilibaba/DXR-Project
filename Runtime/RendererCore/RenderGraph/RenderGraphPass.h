#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/StaticArray.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIResourceViews.h"
#include "RendererCore/RenderGraph/RenderGraphResources.h"
#include "RendererCore/RenderGraph/RenderGraphViews.h"

class FRenderGraphBuilder;
class FRenderGraphPass;
class FRenderGraphViewCache;

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
    FRenderGraphTexture*          Texture                = nullptr;
    FRenderGraphRenderTargetView* RenderTargetView       = nullptr;
    FFloatColor                   ClearValue             = FFloatColor(0.0f, 0.0f, 0.0f, 1.0f);
    FDepthStencilValue            DepthStencilClearValue = FDepthStencilValue(1.0f, 0);
    EAttachmentLoadAction         LoadAction             = EAttachmentLoadAction::DontCare;
    EAttachmentStoreAction        StoreAction            = EAttachmentStoreAction::DontCare;
    bool                          bReadOnly              = false;
};

struct FRenderGraphDepthAttachment
{
    FRenderGraphTexture*          Texture                = nullptr;
    FRenderGraphDepthStencilView* DepthStencilView       = nullptr;
    FDepthStencilValue            DepthStencilClearValue = FDepthStencilValue(1.0f, 0);
    EAttachmentLoadAction         LoadAction             = EAttachmentLoadAction::DontCare;
    EAttachmentStoreAction        StoreAction            = EAttachmentStoreAction::DontCare;
    bool                          bReadOnly              = false;
};

class RENDERERCORE_API FRenderGraphPassResources
{
public:
    static FRenderGraphPassResources Create(const FRenderGraphPass& Pass, FRenderGraphViewCache& ViewCache);

    NODISCARD FRHIBuffer*              Get(FRenderGraphBuffer* Buffer)            const;
    NODISCARD FRHITexture*             Get(FRenderGraphTexture* Texture)          const;
    NODISCARD FRHIShaderResourceView*  Get(FRenderGraphShaderResourceView*  View) const;
    NODISCARD FRHIUnorderedAccessView* Get(FRenderGraphUnorderedAccessView* View) const;
    NODISCARD FRHIRenderTargetView*    Get(FRenderGraphRenderTargetView*    View) const;
    NODISCARD FRHIDepthStencilView*    Get(FRenderGraphDepthStencilView*    View) const;

    NODISCARD FRHIBuffer*              TryGet(FRenderGraphBuffer* Buffer)            const;
    NODISCARD FRHITexture*             TryGet(FRenderGraphTexture* Texture)          const;
    NODISCARD FRHIShaderResourceView*  TryGet(FRenderGraphShaderResourceView*  View) const;
    NODISCARD FRHIUnorderedAccessView* TryGet(FRenderGraphUnorderedAccessView* View) const;
    NODISCARD FRHIRenderTargetView*    TryGet(FRenderGraphRenderTargetView*    View) const;
    NODISCARD FRHIDepthStencilView*    TryGet(FRenderGraphDepthStencilView*    View) const;

    ~FRenderGraphPassResources();

private:
    explicit FRenderGraphPassResources(const FRenderGraphPass& InPass, FRenderGraphViewCache& InViewCache);

    NODISCARD bool IsViewDeclared(const FRenderGraphViewAccess& Access, FRenderGraphShaderResourceView* View)  const;
    NODISCARD bool IsViewDeclared(const FRenderGraphViewAccess& Access, FRenderGraphUnorderedAccessView* View) const;
    NODISCARD bool IsViewDeclared(const FRenderGraphViewAccess& Access, FRenderGraphRenderTargetView* View)    const;
    NODISCARD bool IsViewDeclared(const FRenderGraphViewAccess& Access, FRenderGraphDepthStencilView* View)    const;

    const FRenderGraphPass& Pass;
    FRenderGraphViewCache&  ViewCache;
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
public:
    FRenderGraphPass(const CHAR* InName, ERenderGraphPassFlags InFlags, bool bInEnabled);
    ~FRenderGraphPass();

    void AddTextureAccess(FRenderGraphTexture* Resource, ERHIResourceState State, bool bIsWrite);
    void AddBufferAccess(FRenderGraphBuffer* Resource, ERHIResourceState State, bool bIsWrite);
    void AddViewAccess(FRenderGraphViewAccess Access);
    void BindRenderTarget(uint32 Index, FRenderGraphAttachment Attachment);
    void BindDepthStencil(FRenderGraphDepthAttachment Attachment);

    NODISCARD const CHAR* GetName() const
    {
        return Name;
    }

    NODISCARD ERenderGraphPassFlags GetFlags() const
    {
        return Flags;
    }

    NODISCARD bool IsEnabled() const
    {
        return bIsEnabled;
    }

    NODISCARD bool IsCulled() const
    {
        return bIsCulled;
    }

    NODISCARD bool IsRaster() const
    {
        return IsEnumFlagSet(Flags, ERenderGraphPassFlags::Raster);
    }

    NODISCARD bool HasConflictingAccesses() const
    {
        return bHasConflictingAccesses;
    }

    NODISCARD const TArray<FRenderGraphTextureAccess>&      GetTextureAccesses() const         { return TextureAccesses; }
    NODISCARD const TArray<FRenderGraphBufferAccess>&       GetBufferAccesses() const          { return BufferAccesses; }
    NODISCARD const TArray<FRenderGraphViewAccess>&         GetViewAccesses() const            { return ViewAccesses; }
    NODISCARD const FRenderGraphDepthAttachment&            GetDepthStencil() const            { return DepthStencil; }
    NODISCARD uint32                                        GetNumRenderTargets() const        { return NumRenderTargets; }
    NODISCARD TArray<FRHITransitionBarrierDesc>&            GetTransitions()                   { return Transitions; }
    NODISCARD const TArray<FRHITransitionBarrierDesc>&      GetTransitions() const             { return Transitions; }
    NODISCARD TArray<FRHIUnorderedAccessBarrierDesc>&       GetUnorderedAccessBarriers()       { return UnorderedAccessBarriers; }
    NODISCARD const TArray<FRHIUnorderedAccessBarrierDesc>& GetUnorderedAccessBarriers() const { return UnorderedAccessBarriers; }
    NODISCARD FRenderGraphPassExecutor*                     GetExecutor() const                { return Executor; }

    NODISCARD const TStaticArray<FRenderGraphAttachment, RHI_MAX_RENDER_TARGETS>& GetRenderTargets() const { return RenderTargets; }

    void SetExecutor(FRenderGraphPassExecutor* InExecutor)
    {
        Executor = InExecutor;
    }

    void SetCulled(bool bInCulled)
    {
        bIsCulled = bInCulled;
    }

    void AddTransition(const FRHITransitionBarrierDesc& Desc)
    {
        Transitions.Emplace(Desc);
    }

    void AddUnorderedAccessBarrier(const FRHIUnorderedAccessBarrierDesc& Desc)
    {
        UnorderedAccessBarriers.Emplace(Desc);
    }

private:
    const CHAR*                                                  Name;
    ERenderGraphPassFlags                                        Flags;
    FRenderGraphPassExecutor*                                    Executor;
    TArray<FRenderGraphTextureAccess>                            TextureAccesses;
    TArray<FRenderGraphBufferAccess>                             BufferAccesses;
    TArray<FRenderGraphViewAccess>                               ViewAccesses;
    TStaticArray<FRenderGraphAttachment, RHI_MAX_RENDER_TARGETS> RenderTargets;
    FRenderGraphDepthAttachment                                  DepthStencil;
    uint32                                                       NumRenderTargets;
    TArray<FRHITransitionBarrierDesc>                            Transitions;
    TArray<FRHIUnorderedAccessBarrierDesc>                       UnorderedAccessBarriers;
    bool                                                         bIsCulled;
    bool                                                         bIsEnabled;
    bool                                                         bHasConflictingAccesses;
};

class RENDERERCORE_API FRenderGraphPassBuilder
{
    friend class FRenderGraphBuilder;

    FRenderGraphPassBuilder(FRenderGraphPass& InPass, FRenderGraphBuilder& InBuilder);
    ~FRenderGraphPassBuilder();

public:
    void Read(FRenderGraphShaderResourceView* View, ERHIResourceState State = ERHIResourceState::NonPixelShaderResource);
    void Write(FRenderGraphUnorderedAccessView* View);

    void ReadTexture(FRenderGraphTexture* Texture, ERHIResourceState State);
    void WriteTexture(FRenderGraphTexture* Texture, ERHIResourceState State);

    void ReadBuffer(FRenderGraphBuffer* Buffer, ERHIResourceState State);
    void WriteBuffer(FRenderGraphBuffer* Buffer, ERHIResourceState State);

    void UseDepthStencilView(FRenderGraphDepthStencilView* View, ERHIResourceState State, bool bIsWrite);

    void SetRenderTarget(uint32 Index, FRenderGraphRenderTargetView* View, EAttachmentLoadAction LoadAction = EAttachmentLoadAction::Clear,
        EAttachmentStoreAction StoreAction = EAttachmentStoreAction::Store, const FFloatColor& ClearValue = FFloatColor(0.0f, 0.0f, 0.0f, 1.0f));

    void SetDepthStencil(FRenderGraphDepthStencilView* View, EAttachmentLoadAction LoadAction = EAttachmentLoadAction::Clear, 
        EAttachmentStoreAction StoreAction = EAttachmentStoreAction::Store, const FDepthStencilValue& ClearValue = FDepthStencilValue(1.0f, 0));

    void SetRenderTarget(uint32 Index, FRenderGraphTexture* Texture, EAttachmentLoadAction LoadAction = EAttachmentLoadAction::Clear, 
        EAttachmentStoreAction StoreAction = EAttachmentStoreAction::Store, const FFloatColor& ClearValue = FFloatColor(0.0f, 0.0f, 0.0f, 1.0f));

    void SetDepthStencil(FRenderGraphTexture* Texture, EAttachmentLoadAction LoadAction = EAttachmentLoadAction::Clear, 
        EAttachmentStoreAction StoreAction = EAttachmentStoreAction::Store, const FDepthStencilValue& ClearValue = FDepthStencilValue(1.0f, 0), bool bReadOnly = false);

private:
    FRenderGraphPass&    Pass;
    FRenderGraphBuilder& Builder;
};
