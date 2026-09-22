#include "Core/Math/Math.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "RendererCore/RenderGraph/RenderGraphBuilder.h"
#include "RendererCore/RenderGraph/RenderGraphPass.h"
#include "RendererCore/RenderGraph/RenderGraphViewCache.h"

static bool BufferIsReadThroughShaderResourceView(ERHIResourceState State)
{
    return State == ERHIResourceState::NonPixelShaderResource || State == ERHIResourceState::PixelShaderResource || State == ERHIResourceState::ShaderResource || State == ERHIResourceState::GenericRead;
}

FRenderGraphPassResources FRenderGraphPassResources::Create(const FRenderGraphPass& Pass, FRenderGraphViewCache& ViewCache)
{
    return FRenderGraphPassResources(Pass, ViewCache);
}

FRenderGraphPassResources::FRenderGraphPassResources(const FRenderGraphPass& InPass, FRenderGraphViewCache& InViewCache)
    : Pass(InPass)
    , ViewCache(InViewCache)
{
}

FRenderGraphPassResources::~FRenderGraphPassResources() = default;

FRenderGraphPass::FRenderGraphPass(const CHAR* InName, ERenderGraphPassFlags InFlags, bool bInEnabled)
    : Name(InName ? InName : "RenderGraphPass")
    , Flags(InFlags)
    , Executor(nullptr)
    , TextureAccesses()
    , BufferAccesses()
    , ViewAccesses()
    , RenderTargets()
    , DepthStencil()
    , NumRenderTargets(0)
    , Transitions()
    , UnorderedAccessBarriers()
    , bIsCulled(false)
    , bIsEnabled(bInEnabled)
    , bHasConflictingAccesses(false)
{
}

FRenderGraphPass::~FRenderGraphPass() = default;

void FRenderGraphPass::AddTextureAccess(FRenderGraphTexture* Resource, ERHIResourceState State, bool bIsWrite)
{
    if (!Resource)
    {
        return;
    }

    for (FRenderGraphTextureAccess& Access : TextureAccesses)
    {
        if (Access.Resource != Resource)
        {
            continue;
        }

        if (!Access.bIsWrite && !bIsWrite)
        {
            Access.State |= State;
            return;
        }

        if (Access.State != State)
        {
            LOG_ERROR("Pass '%s' declares conflicting accesses on '%s', %s and %s. A pass can only use " "a resource one way, so this needs to be split into two passes.", Name, Resource->GetName(), ToString(Access.State), ToString(State));
            bHasConflictingAccesses = true;
            return;
        }

        Access.bIsWrite = true;
        return;
    }

    FRenderGraphTextureAccess& Access = TextureAccesses.Emplace();
    Access.Resource                   = Resource;
    Access.State                      = State;
    Access.bIsWrite                   = bIsWrite;
}

void FRenderGraphPass::AddBufferAccess(FRenderGraphBuffer* Resource, ERHIResourceState State, bool bIsWrite)
{
    if (!Resource)
    {
        return;
    }

    for (FRenderGraphBufferAccess& Access : BufferAccesses)
    {
        if (Access.Resource != Resource)
        {
            continue;
        }

        if (!Access.bIsWrite && !bIsWrite)
        {
            Access.State |= State;
            return;
        }

        if (Access.State != State)
        {
            LOG_ERROR("Pass '%s' declares conflicting accesses on '%s', %s and %s. A pass can only use " "a resource one way, so this needs to be split into two passes.", Name, Resource->GetName(), ToString(Access.State), ToString(State));
            bHasConflictingAccesses = true;
            return;
        }

        Access.bIsWrite = true;
        return;
    }

    FRenderGraphBufferAccess& Access = BufferAccesses.Emplace();
    Access.Resource                  = Resource;
    Access.State                     = State;
    Access.bIsWrite                  = bIsWrite;
}

void FRenderGraphPass::AddViewAccess(FRenderGraphViewAccess Access)
{
    for (FRenderGraphViewAccess& ExistingAccess : ViewAccesses)
    {
        if (ExistingAccess.Type != Access.Type)
        {
            continue;
        }

        const bool bSameView = (Access.Type == ERenderGraphViewAccessType::ShaderResource && ExistingAccess.ShaderResourceView == Access.ShaderResourceView)
            || (Access.Type == ERenderGraphViewAccessType::UnorderedAccess && ExistingAccess.UnorderedAccessView == Access.UnorderedAccessView)
            || (Access.Type == ERenderGraphViewAccessType::RenderTarget && ExistingAccess.RenderTargetView == Access.RenderTargetView)
            || (Access.Type == ERenderGraphViewAccessType::DepthStencil && ExistingAccess.DepthStencilView == Access.DepthStencilView);

        if (!bSameView)
        {
            continue;
        }

        if (!ExistingAccess.bIsWrite && !Access.bIsWrite)
        {
            ExistingAccess.State |= Access.State;
            return;
        }

        if (ExistingAccess.State != Access.State)
        {
            LOG_ERROR("Pass '%s' declares conflicting accesses on view '%s'", Name, Access.bIsTextureParent ? Access.ParentTexture->GetName() : Access.ParentBuffer->GetName());
            bHasConflictingAccesses = true;
            return;
        }

        ExistingAccess.bIsWrite = ExistingAccess.bIsWrite || Access.bIsWrite;
        return;
    }

    ViewAccesses.Emplace(Move(Access));
}

void FRenderGraphPass::BindRenderTarget(uint32 Index, FRenderGraphAttachment Attachment)
{
    RenderTargets[Index] = Attachment;
    NumRenderTargets     = Math::Max(NumRenderTargets, Index + 1);
}

void FRenderGraphPass::BindDepthStencil(FRenderGraphDepthAttachment Attachment)
{
    DepthStencil = Attachment;
}

FRenderGraphPassBuilder::FRenderGraphPassBuilder(FRenderGraphPass& InPass, FRenderGraphBuilder& InBuilder)
    : Pass(InPass)
    , Builder(InBuilder)
{
}

FRenderGraphPassBuilder::~FRenderGraphPassBuilder() = default;

void FRenderGraphPassBuilder::Read(FRenderGraphShaderResourceView* View, ERHIResourceState State)
{
    if (!View)
    {
        LOG_ERROR("Pass '%s' declared a read on a null SRV", Pass.GetName());
        return;
    }

    FRenderGraphViewAccess Access;
    Access.Type               = ERenderGraphViewAccessType::ShaderResource;
    Access.ShaderResourceView = View;
    Access.State              = State;
    Access.bIsWrite           = false;

    if (View->GetParentKind() == ERenderGraphParentKind::Texture)
    {
        Access.bIsTextureParent = true;
        Access.ParentTexture    = View->GetParentTexture();
        Access.SubresourceRange = RenderGraphViewRanges::SubresourceRangeFor(Access.ParentTexture, View->GetDesc());

        Pass.AddTextureAccess(Access.ParentTexture, State, false);
    }
    else
    {
        Access.bIsTextureParent = false;
        Access.ParentBuffer     = View->GetParentBuffer();
        Access.BufferRange      = RenderGraphViewRanges::BufferRangeFor(Access.ParentBuffer, View->GetDesc());

        Pass.AddBufferAccess(Access.ParentBuffer, State, false);
    }

    Pass.AddViewAccess(Move(Access));
}

void FRenderGraphPassBuilder::Write(FRenderGraphUnorderedAccessView* View)
{
    if (!View)
    {
        LOG_ERROR("Pass '%s' declared a write on a null UAV", Pass.GetName());
        return;
    }

    FRenderGraphViewAccess Access;
    Access.Type                = ERenderGraphViewAccessType::UnorderedAccess;
    Access.UnorderedAccessView = View;
    Access.State               = ERHIResourceState::UnorderedAccess;
    Access.bIsWrite            = true;

    if (View->GetParentKind() == ERenderGraphParentKind::Texture)
    {
        Access.bIsTextureParent = true;
        Access.ParentTexture    = View->GetParentTexture();
        Access.SubresourceRange = RenderGraphViewRanges::SubresourceRangeFor(Access.ParentTexture, View->GetDesc());

        Pass.AddTextureAccess(Access.ParentTexture, ERHIResourceState::UnorderedAccess, true);
    }
    else
    {
        Access.bIsTextureParent = false;
        Access.ParentBuffer     = View->GetParentBuffer();
        Access.BufferRange      = RenderGraphViewRanges::BufferRangeFor(Access.ParentBuffer, View->GetDesc());

        Pass.AddBufferAccess(Access.ParentBuffer, ERHIResourceState::UnorderedAccess, true);
    }

    Pass.AddViewAccess(Move(Access));
}

void FRenderGraphPassBuilder::ReadTexture(FRenderGraphTexture* Texture, ERHIResourceState State)
{
    if (!Texture)
    {
        LOG_ERROR("Pass '%s' declared a read on a null texture", Pass.GetName());
        return;
    }

    Pass.AddTextureAccess(Texture, State, false);

    FRenderGraphShaderResourceView* DefaultSRV = Builder.GetOrCreateDefaultSRV(Texture);

    FRenderGraphViewAccess Access;
    Access.Type               = ERenderGraphViewAccessType::ShaderResource;
    Access.ShaderResourceView = DefaultSRV;
    Access.State              = State;
    Access.bIsWrite           = false;
    Access.bIsTextureParent   = true;
    Access.ParentTexture      = Texture;
    Access.SubresourceRange   = RenderGraphViewRanges::SubresourceRangeFor(Texture, DefaultSRV->GetDesc());

    Pass.AddViewAccess(Move(Access));
}

void FRenderGraphPassBuilder::WriteTexture(FRenderGraphTexture* Texture, ERHIResourceState State)
{
    if (!Texture)
    {
        LOG_ERROR("Pass '%s' declared a write on a null texture", Pass.GetName());
        return;
    }

    Pass.AddTextureAccess(Texture, State, true);

    if (State == ERHIResourceState::UnorderedAccess)
    {
        FRenderGraphUnorderedAccessView* DefaultUAV = Builder.GetOrCreateDefaultUAV(Texture);

        FRenderGraphViewAccess Access;
        Access.Type              = ERenderGraphViewAccessType::UnorderedAccess;
        Access.UnorderedAccessView = DefaultUAV;
        Access.State               = ERHIResourceState::UnorderedAccess;
        Access.bIsWrite            = true;
        Access.bIsTextureParent    = true;
        Access.ParentTexture       = Texture;
        Access.SubresourceRange    = RenderGraphViewRanges::SubresourceRangeFor(Texture, DefaultUAV->GetDesc());

        Pass.AddViewAccess(Move(Access));
    }
}

void FRenderGraphPassBuilder::ReadBuffer(FRenderGraphBuffer* Buffer, ERHIResourceState State)
{
    if (!Buffer)
    {
        LOG_ERROR("Pass '%s' declared a read on a null buffer", Pass.GetName());
        return;
    }

    Pass.AddBufferAccess(Buffer, State, false);

    if (!BufferIsReadThroughShaderResourceView(State))
    {
        return;
    }

    FRenderGraphShaderResourceView* DefaultSRV = Builder.GetOrCreateDefaultSRV(Buffer);

    FRenderGraphViewAccess Access;
    Access.Type               = ERenderGraphViewAccessType::ShaderResource;
    Access.ShaderResourceView = DefaultSRV;
    Access.State              = State;
    Access.bIsWrite           = false;
    Access.bIsTextureParent   = false;
    Access.ParentBuffer       = Buffer;
    Access.BufferRange        = RenderGraphViewRanges::BufferRangeFor(Buffer, DefaultSRV->GetDesc());

    Pass.AddViewAccess(Move(Access));
}

void FRenderGraphPassBuilder::WriteBuffer(FRenderGraphBuffer* Buffer, ERHIResourceState State)
{
    if (!Buffer)
    {
        LOG_ERROR("Pass '%s' declared a write on a null buffer", Pass.GetName());
        return;
    }

    Pass.AddBufferAccess(Buffer, State, true);

    if (State == ERHIResourceState::UnorderedAccess)
    {
        FRenderGraphUnorderedAccessView* DefaultUAV = Builder.GetOrCreateDefaultUAV(Buffer);

        FRenderGraphViewAccess Access;
        Access.Type              = ERenderGraphViewAccessType::UnorderedAccess;
        Access.UnorderedAccessView = DefaultUAV;
        Access.State               = ERHIResourceState::UnorderedAccess;
        Access.bIsWrite            = true;
        Access.bIsTextureParent    = false;
        Access.ParentBuffer        = Buffer;
        Access.BufferRange         = RenderGraphViewRanges::BufferRangeFor(Buffer, DefaultUAV->GetDesc());

        Pass.AddViewAccess(Move(Access));
    }
}

void FRenderGraphPassBuilder::SetRenderTarget(uint32 Index, FRenderGraphRenderTargetView* View, EAttachmentLoadAction LoadAction, EAttachmentStoreAction StoreAction, const FFloatColor& ClearValue)
{
    if (!View)
    {
        LOG_ERROR("Pass '%s' bound a null render-target view", Pass.GetName());
        return;
    }

    if (!Pass.IsRaster())
    {
        LOG_ERROR("Pass '%s' bound a render-target but is not flagged ERenderGraphPassFlags::Raster", Pass.GetName());
        return;
    }

    if (Index >= RHI_MAX_RENDER_TARGETS)
    {
        LOG_ERROR("Pass '%s' bound render-target %u, but the maximum is %d", Pass.GetName(), Index, RHI_MAX_RENDER_TARGETS);
        return;
    }

    FRenderGraphTexture* Texture = View->GetParent();
    if (!Texture->GetDesc().TextureDesc.IsRenderTarget())
    {
        LOG_ERROR("Pass '%s' bound texture '%s' as a render-target, but it lacks ETextureUsageFlags::RenderTarget", Pass.GetName(), Texture->GetName());
        return;
    }

    FRenderGraphAttachment Attachment;
    Attachment.Texture          = Texture;
    Attachment.RenderTargetView = View;
    Attachment.ClearValue       = ClearValue;
    Attachment.LoadAction       = LoadAction;
    Attachment.StoreAction      = StoreAction;

    Pass.BindRenderTarget(Index, Attachment);

    FRenderGraphViewAccess Access;
    Access.Type             = ERenderGraphViewAccessType::RenderTarget;
    Access.RenderTargetView = View;
    Access.State            = ERHIResourceState::RenderTarget;
    Access.bIsWrite         = true;
    Access.bIsTextureParent = true;
    Access.ParentTexture    = Texture;
    Access.SubresourceRange = RenderGraphViewRanges::SubresourceRangeFor(Texture, View->GetDesc());

    Pass.AddViewAccess(Move(Access));
    Pass.AddTextureAccess(Texture, ERHIResourceState::RenderTarget, true);
}

void FRenderGraphPassBuilder::SetDepthStencil(FRenderGraphDepthStencilView* View, EAttachmentLoadAction LoadAction, EAttachmentStoreAction StoreAction, const FDepthStencilValue& ClearValue)
{
    if (!View)
    {
        LOG_ERROR("Pass '%s' bound a null depth-stencil view", Pass.GetName());
        return;
    }

    if (!Pass.IsRaster())
    {
        LOG_ERROR("Pass '%s' bound a depth-stencil but is not flagged ERenderGraphPassFlags::Raster", Pass.GetName());
        return;
    }

    FRenderGraphTexture* Texture = View->GetParent();
    if (!Texture->GetDesc().TextureDesc.IsDepthStencil())
    {
        LOG_ERROR("Pass '%s' bound texture '%s' as a depth-stencil, but it lacks ETextureUsageFlags::DepthStencil", Pass.GetName(), Texture->GetName());
        return;
    }

    const bool bReadOnly = IsEnumFlagSet(View->GetDesc().Flags, EDepthStencilViewFlags::ReadOnlyAll);

    FRenderGraphDepthAttachment Attachment;
    Attachment.Texture               = Texture;
    Attachment.DepthStencilView      = View;
    Attachment.DepthStencilClearValue = ClearValue;
    Attachment.LoadAction            = LoadAction;
    Attachment.StoreAction           = StoreAction;
    Attachment.bReadOnly             = bReadOnly;
    Pass.BindDepthStencil(Attachment);

    FRenderGraphViewAccess Access;
    Access.Type             = ERenderGraphViewAccessType::DepthStencil;
    Access.DepthStencilView = View;
    Access.bIsWrite         = !bReadOnly;
    Access.bIsTextureParent = true;
    Access.ParentTexture    = Texture;
    Access.SubresourceRange = RenderGraphViewRanges::SubresourceRangeFor(Texture, View->GetDesc());
    Access.State            = bReadOnly ? ERHIResourceState::DepthRead : ERHIResourceState::DepthWrite;

    Pass.AddViewAccess(Move(Access));
    Pass.AddTextureAccess(Texture, Access.State, !bReadOnly);
}

void FRenderGraphPassBuilder::UseDepthStencilView(FRenderGraphDepthStencilView* View, ERHIResourceState State, bool bIsWrite)
{
    if (!View)
    {
        LOG_ERROR("Pass '%s' declared access on a null depth-stencil view", Pass.GetName());
        return;
    }

    FRenderGraphTexture* Texture = View->GetParent();
    if (!Texture->GetDesc().TextureDesc.IsDepthStencil())
    {
        LOG_ERROR("Pass '%s' declared access on texture '%s' as a depth-stencil view, but it lacks ETextureUsageFlags::DepthStencil", Pass.GetName(), Texture->GetName());
        return;
    }

    FRenderGraphViewAccess Access;
    Access.Type             = ERenderGraphViewAccessType::DepthStencil;
    Access.DepthStencilView = View;
    Access.State            = State;
    Access.bIsWrite         = bIsWrite;
    Access.bIsTextureParent = true;
    Access.ParentTexture    = Texture;
    Access.SubresourceRange = RenderGraphViewRanges::SubresourceRangeFor(Texture, View->GetDesc());
    Pass.AddViewAccess(Move(Access));

    Pass.AddTextureAccess(Texture, State, bIsWrite);
}

void FRenderGraphPassBuilder::SetRenderTarget(uint32 Index, FRenderGraphTexture* Texture, EAttachmentLoadAction LoadAction, EAttachmentStoreAction StoreAction, const FFloatColor& ClearValue)
{
    if (!Texture)
    {
        LOG_ERROR("Pass '%s' bound a null render-target", Pass.GetName());
        return;
    }

    SetRenderTarget(Index, Builder.GetOrCreateDefaultRTV(Texture), LoadAction, StoreAction, ClearValue);
}

void FRenderGraphPassBuilder::SetDepthStencil(FRenderGraphTexture* Texture, EAttachmentLoadAction LoadAction, EAttachmentStoreAction StoreAction, const FDepthStencilValue& ClearValue, bool bReadOnly)
{
    if (!Texture)
    {
        LOG_ERROR("Pass '%s' bound a null depth-stencil", Pass.GetName());
        return;
    }

    // ReadOnlyStencil on a depth-only format is a contradiction that the backends drop anyway, so only name the
    // aspects the format actually carries
    EDepthStencilViewFlags Flags = EDepthStencilViewFlags::None;
    if (bReadOnly)
    {
        Flags = IsStencilFormat(Texture->GetDesc().TextureDesc.Format)
            ? EDepthStencilViewFlags::ReadOnlyAll
            : EDepthStencilViewFlags::ReadOnlyDepth;
    }

    SetDepthStencil(Builder.GetOrCreateDefaultDSV(Texture, Flags), LoadAction, StoreAction, ClearValue);
}

FRHITexture* FRenderGraphPassResources::TryGet(FRenderGraphTexture* Texture) const
{
    if (!Texture)
    {
        return nullptr;
    }

    for (const FRenderGraphTextureAccess& Access : Pass.GetTextureAccesses())
    {
        if (Access.Resource == Texture)
        {
            return Texture->GetRHITexture();
        }
    }

    return nullptr;
}

FRHITexture* FRenderGraphPassResources::Get(FRenderGraphTexture* Texture) const
{
    if (!Texture)
    {
        LOG_ERROR("Pass '%s' resolved a null texture", Pass.GetName());
        return nullptr;
    }

    if (FRHITexture* ResolvedTexture = TryGet(Texture))
    {
        return ResolvedTexture;
    }

    LOG_ERROR("Pass '%s' resolved texture '%s', which it never declared a read or write on", Pass.GetName(), Texture->GetName());
    return nullptr;
}

FRHIBuffer* FRenderGraphPassResources::TryGet(FRenderGraphBuffer* Buffer) const
{
    if (!Buffer)
    {
        return nullptr;
    }

    for (const FRenderGraphBufferAccess& Access : Pass.GetBufferAccesses())
    {
        if (Access.Resource == Buffer)
        {
            return Buffer->GetRHIBuffer();
        }
    }

    return nullptr;
}

FRHIBuffer* FRenderGraphPassResources::Get(FRenderGraphBuffer* Buffer) const
{
    if (!Buffer)
    {
        LOG_ERROR("Pass '%s' resolved a null buffer", Pass.GetName());
        return nullptr;
    }

    if (FRHIBuffer* ResolvedBuffer = TryGet(Buffer))
    {
        return ResolvedBuffer;
    }

    LOG_ERROR("Pass '%s' resolved buffer '%s', which it never declared a read or write on", Pass.GetName(), Buffer->GetName());
    return nullptr;
}

bool FRenderGraphPassResources::IsViewDeclared(const FRenderGraphViewAccess& Access, FRenderGraphShaderResourceView* View) const
{
    return Access.Type == ERenderGraphViewAccessType::ShaderResource && Access.ShaderResourceView == View;
}

bool FRenderGraphPassResources::IsViewDeclared(const FRenderGraphViewAccess& Access, FRenderGraphUnorderedAccessView* View) const
{
    return Access.Type == ERenderGraphViewAccessType::UnorderedAccess && Access.UnorderedAccessView == View;
}

bool FRenderGraphPassResources::IsViewDeclared(const FRenderGraphViewAccess& Access, FRenderGraphRenderTargetView* View) const
{
    return Access.Type == ERenderGraphViewAccessType::RenderTarget && Access.RenderTargetView == View;
}

bool FRenderGraphPassResources::IsViewDeclared(const FRenderGraphViewAccess& Access, FRenderGraphDepthStencilView* View) const
{
    return Access.Type == ERenderGraphViewAccessType::DepthStencil && Access.DepthStencilView == View;
}

FRHIShaderResourceView* FRenderGraphPassResources::TryGet(FRenderGraphShaderResourceView* View) const
{
    if (!View)
    {
        return nullptr;
    }

    for (const FRenderGraphViewAccess& Access : Pass.GetViewAccesses())
    {
        if (IsViewDeclared(Access, View))
        {
            return ViewCache.GetOrCreate(View);
        }
    }

    return nullptr;
}

FRHIShaderResourceView* FRenderGraphPassResources::Get(FRenderGraphShaderResourceView* View) const
{
    if (!View)
    {
        LOG_ERROR("Pass '%s' resolved a null SRV", Pass.GetName());
        return nullptr;
    }

    if (FRHIShaderResourceView* ResolvedView = TryGet(View))
    {
        return ResolvedView;
    }

    LOG_ERROR("Pass '%s' resolved SRV '%s', which it never declared a read on", Pass.GetName(), View->GetName());
    return nullptr;
}

FRHIUnorderedAccessView* FRenderGraphPassResources::TryGet(FRenderGraphUnorderedAccessView* View) const
{
    if (!View)
    {
        return nullptr;
    }

    for (const FRenderGraphViewAccess& Access : Pass.GetViewAccesses())
    {
        if (IsViewDeclared(Access, View))
        {
            return ViewCache.GetOrCreate(View);
        }
    }

    return nullptr;
}

FRHIUnorderedAccessView* FRenderGraphPassResources::Get(FRenderGraphUnorderedAccessView* View) const
{
    if (!View)
    {
        LOG_ERROR("Pass '%s' resolved a null UAV", Pass.GetName());
        return nullptr;
    }

    if (FRHIUnorderedAccessView* ResolvedView = TryGet(View))
    {
        return ResolvedView;
    }

    LOG_ERROR("Pass '%s' resolved UAV '%s', which it never declared a write on", Pass.GetName(), View->GetName());
    return nullptr;
}

FRHIRenderTargetView* FRenderGraphPassResources::TryGet(FRenderGraphRenderTargetView* View) const
{
    if (!View)
    {
        return nullptr;
    }

    for (const FRenderGraphViewAccess& Access : Pass.GetViewAccesses())
    {
        if (IsViewDeclared(Access, View))
        {
            return ViewCache.GetOrCreate(View);
        }
    }

    return nullptr;
}

FRHIRenderTargetView* FRenderGraphPassResources::Get(FRenderGraphRenderTargetView* View) const
{
    if (!View)
    {
        LOG_ERROR("Pass '%s' resolved a null RTV", Pass.GetName());
        return nullptr;
    }

    if (FRHIRenderTargetView* ResolvedView = TryGet(View))
    {
        return ResolvedView;
    }

    LOG_ERROR("Pass '%s' resolved RTV '%s', which it never declared as a render target", Pass.GetName(), View->GetName());
    return nullptr;
}

FRHIDepthStencilView* FRenderGraphPassResources::TryGet(FRenderGraphDepthStencilView* View) const
{
    if (!View)
    {
        return nullptr;
    }

    for (const FRenderGraphViewAccess& Access : Pass.GetViewAccesses())
    {
        if (IsViewDeclared(Access, View))
        {
            return ViewCache.GetOrCreate(View);
        }
    }

    return nullptr;
}

FRHIDepthStencilView* FRenderGraphPassResources::Get(FRenderGraphDepthStencilView* View) const
{
    if (!View)
    {
        LOG_ERROR("Pass '%s' resolved a null DSV", Pass.GetName());
        return nullptr;
    }

    if (FRHIDepthStencilView* ResolvedView = TryGet(View))
    {
        return ResolvedView;
    }

    LOG_ERROR("Pass '%s' resolved DSV '%s', which it never declared as a depth-stencil attachment", Pass.GetName(), View->GetName());
    return nullptr;
}
