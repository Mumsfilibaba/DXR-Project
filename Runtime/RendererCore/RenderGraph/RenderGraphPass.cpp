#include "Core/Math/Math.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "RendererCore/RenderGraph/RenderGraphPass.h"

template<typename AccessType, typename ResourceType>
static void AddResourceAccess(TArray<AccessType>& Accesses, const CHAR* PassName, const CHAR* ResourceName,
    ResourceType* Resource, ERHIResourceState State, bool bIsWrite)
{
    for (AccessType& Access : Accesses)
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
            LOG_ERROR("Pass '%s' declares conflicting accesses on '%s', %s and %s. A pass can only use "
                "a resource one way, so this needs to be split into two passes.", PassName, ResourceName, ToString(Access.State), ToString(State));
            return;
        }

        Access.bIsWrite = true;
        return;
    }

    AccessType& Access = Accesses.Emplace();
    Access.Resource    = Resource;
    Access.State       = State;
    Access.bIsWrite    = bIsWrite;
}

void FRenderGraphPassBuilder::ReadTexture(FRenderGraphTexture* Texture, ERHIResourceState State)
{
    if (!Texture)
    {
        LOG_ERROR("Pass '%s' declared a read on a null texture", Pass.GetName());
        return;
    }

    AddResourceAccess(Pass.TextureAccesses, Pass.GetName(), Texture->GetName(), Texture, State, false);
}

void FRenderGraphPassBuilder::WriteTexture(FRenderGraphTexture* Texture, ERHIResourceState State)
{
    if (!Texture)
    {
        LOG_ERROR("Pass '%s' declared a write on a null texture", Pass.GetName());
        return;
    }

    AddResourceAccess(Pass.TextureAccesses, Pass.GetName(), Texture->GetName(), Texture, State, true);
}

void FRenderGraphPassBuilder::ReadBuffer(FRenderGraphBuffer* Buffer, ERHIResourceState State)
{
    if (!Buffer)
    {
        LOG_ERROR("Pass '%s' declared a read on a null buffer", Pass.GetName());
        return;
    }

    AddResourceAccess(Pass.BufferAccesses, Pass.GetName(), Buffer->GetName(), Buffer, State, false);
}

void FRenderGraphPassBuilder::WriteBuffer(FRenderGraphBuffer* Buffer, ERHIResourceState State)
{
    if (!Buffer)
    {
        LOG_ERROR("Pass '%s' declared a write on a null buffer", Pass.GetName());
        return;
    }

    AddResourceAccess(Pass.BufferAccesses, Pass.GetName(), Buffer->GetName(), Buffer, State, true);
}

void FRenderGraphPassBuilder::SetRenderTarget(uint32 Index, FRenderGraphTexture* Texture, EAttachmentLoadAction LoadAction,
    EAttachmentStoreAction StoreAction, const FFloatColor& ClearValue)
{
    if (!Texture)
    {
        LOG_ERROR("Pass '%s' bound a null render-target", Pass.GetName());
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

    if (!Texture->GetDesc().TextureDesc.IsRenderTarget())
    {
        LOG_ERROR("Pass '%s' bound texture '%s' as a render-target, but it lacks ETextureUsageFlags::RenderTarget",
            Pass.GetName(), Texture->GetName());
        return;
    }

    FRenderGraphAttachment& Attachment = Pass.RenderTargets[Index];
    Attachment.Texture     = Texture;
    Attachment.ClearValue  = ClearValue;
    Attachment.LoadAction  = LoadAction;
    Attachment.StoreAction = StoreAction;

    Pass.NumRenderTargets = Math::Max(Pass.NumRenderTargets, Index + 1);

    WriteTexture(Texture, ERHIResourceState::RenderTarget);
}

void FRenderGraphPassBuilder::SetDepthStencil(FRenderGraphTexture* Texture, EAttachmentLoadAction LoadAction, EAttachmentStoreAction StoreAction,
    const FDepthStencilValue& ClearValue, bool bReadOnly)
{
    if (!Texture)
    {
        LOG_ERROR("Pass '%s' bound a null depth-stencil", Pass.GetName());
        return;
    }

    if (!Pass.IsRaster())
    {
        LOG_ERROR("Pass '%s' bound a depth-stencil but is not flagged ERenderGraphPassFlags::Raster", Pass.GetName());
        return;
    }

    if (!Texture->GetDesc().TextureDesc.IsDepthStencil())
    {
        LOG_ERROR("Pass '%s' bound texture '%s' as a depth-stencil, but it lacks ETextureUsageFlags::DepthStencil", Pass.GetName(), Texture->GetName());
        return;
    }

    Pass.DepthStencil.Texture                = Texture;
    Pass.DepthStencil.DepthStencilClearValue = ClearValue;
    Pass.DepthStencil.LoadAction             = LoadAction;
    Pass.DepthStencil.StoreAction            = StoreAction;
    Pass.DepthStencil.bReadOnly              = bReadOnly;

    if (bReadOnly)
    {
        ReadTexture(Texture, ERHIResourceState::DepthRead);
    }
    else
    {
        WriteTexture(Texture, ERHIResourceState::DepthWrite);
    }
}

FRHITexture* FRenderGraphPassResources::Get(FRenderGraphTexture* Texture) const
{
    if (!Texture)
    {
        LOG_ERROR("Pass '%s' resolved a null texture", Pass.GetName());
        return nullptr;
    }

    for (const FRenderGraphTextureAccess& Access : Pass.TextureAccesses)
    {
        if (Access.Resource == Texture)
        {
            return Texture->GetRHITexture();
        }
    }

    LOG_ERROR("Pass '%s' resolved texture '%s', which it never declared a read or write on", Pass.GetName(), Texture->GetName());
    return nullptr;
}

FRHIBuffer* FRenderGraphPassResources::Get(FRenderGraphBuffer* Buffer) const
{
    if (!Buffer)
    {
        LOG_ERROR("Pass '%s' resolved a null buffer", Pass.GetName());
        return nullptr;
    }

    for (const FRenderGraphBufferAccess& Access : Pass.BufferAccesses)
    {
        if (Access.Resource == Buffer)
        {
            return Buffer->GetRHIBuffer();
        }
    }

    LOG_ERROR("Pass '%s' resolved buffer '%s', which it never declared a read or write on", Pass.GetName(), Buffer->GetName());
    return nullptr;
}
