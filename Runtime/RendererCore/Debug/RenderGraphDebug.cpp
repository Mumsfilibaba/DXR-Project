#include "Core/Containers/Map.h"
#include "Core/Containers/Pair.h"
#include "Core/Threading/ScopedLock.h"
#include "RendererCore/Debug/RenderGraphDebug.h"

static bool WriteLoadsPreviousContents(const FRenderGraphPass& Pass, const FRenderGraphTexture* Texture)
{
    const TStaticArray<FRenderGraphAttachment, RHI_MAX_RENDER_TARGETS>& RenderTargets = Pass.GetRenderTargets();
    for (int32 Index = 0; Index < static_cast<int32>(Pass.GetNumRenderTargets()); ++Index)
    {
        if (RenderTargets[Index].Texture == Texture)
        {
            return RenderTargets[Index].LoadAction == EAttachmentLoadAction::Load;
        }
    }

    const FRenderGraphDepthAttachment& DepthStencil = Pass.GetDepthStencil();
    if (DepthStencil.Texture == Texture)
    {
        return DepthStencil.LoadAction == EAttachmentLoadAction::Load;
    }

    return true;
}

AtomicBool                RenderGraphDebug::bCaptureEnabled(false);
FSpinLock                 RenderGraphDebug::SnapshotLock;
FRenderGraphDebugSnapshot RenderGraphDebug::LatestSnapshot;
bool                      RenderGraphDebug::bHasSnapshot = false;

void RenderGraphDebug::SetCaptureEnabled(bool bEnabled)
{
    bCaptureEnabled.Store(bEnabled);
}

bool RenderGraphDebug::IsCaptureEnabled()
{
    return bCaptureEnabled.Load();
}

void RenderGraphDebug::Publish(FRenderGraphDebugSnapshot&& Snapshot)
{
    TScopedLock Lock(SnapshotLock);
    LatestSnapshot = ::Move(Snapshot);
    bHasSnapshot   = true;
}

bool RenderGraphDebug::CopyLatest(FRenderGraphDebugSnapshot& Out)
{
    TScopedLock Lock(SnapshotLock);

    if (!bHasSnapshot)
    {
        return false;
    }

    Out = LatestSnapshot;
    return true;
}

void RenderGraphDebug::CaptureSnapshot(const FRenderGraphBuilder& Builder, FRenderGraphDebugSnapshot& Out)
{
    Out = FRenderGraphDebugSnapshot();

    Out.GraphName  = Builder.GetName() ? Builder.GetName() : "";
    Out.Statistics = Builder.GetStatistics();

    TMap<const FRenderGraphTexture*, int32> TextureIndices;
    TMap<const FRenderGraphBuffer*, int32>  BufferIndices;

    const TArray<FRenderGraphTexture*>& Textures = Builder.GetTextures();
    Out.Resources.Reserve(Textures.Size() + Builder.GetBuffers().Size());

    for (FRenderGraphTexture* Texture : Textures)
    {
        if (!Texture)
        {
            continue;
        }

        FRenderGraphDebugResource& Resource = Out.Resources.Emplace();
        Resource.Name        = Texture->GetName() ? Texture->GetName() : "UnnamedTexture";
        Resource.Kind        = ERenderGraphDebugResourceKind::Texture;
        Resource.bIsExternal = Texture->IsExternal();
        Resource.Width       = static_cast<uint32>(Texture->GetDesc().TextureDesc.Extent.X);
        Resource.Height      = static_cast<uint32>(Texture->GetDesc().TextureDesc.Extent.Y);
        Resource.DepthOrSize = static_cast<uint32>(Texture->GetDesc().TextureDesc.Extent.Z);

        TextureIndices.Add(Texture, Out.Resources.Size() - 1);
    }

    for (FRenderGraphBuffer* Buffer : Builder.GetBuffers())
    {
        if (!Buffer)
        {
            continue;
        }

        FRenderGraphDebugResource& Resource = Out.Resources.Emplace();
        Resource.Name        = Buffer->GetName() ? Buffer->GetName() : "UnnamedBuffer";
        Resource.Kind        = ERenderGraphDebugResourceKind::Buffer;
        Resource.bIsExternal = Buffer->IsExternal();
        Resource.Width       = 0;
        Resource.Height      = 0;
        Resource.DepthOrSize = static_cast<uint32>(Buffer->GetDesc().BufferDesc.Size);

        BufferIndices.Add(Buffer, Out.Resources.Size() - 1);
    }

    const TArray<FRenderGraphPass*>& Passes = Builder.GetPasses();
    Out.Passes.Reserve(Passes.Size());

    // Last writer of each resource: (PassIndex, AccessIndex)
    TMap<int32, TPair<int32, int32>> LastWriterByResource;

    for (int32 PassIndex = 0; PassIndex < Passes.Size(); ++PassIndex)
    {
        const FRenderGraphPass* Pass = Passes[PassIndex];
        if (!Pass)
        {
            continue;
        }

        FRenderGraphDebugPass& DebugPass = Out.Passes.Emplace();
        DebugPass.Name     = Pass->GetName() ? Pass->GetName() : "UnnamedPass";
        DebugPass.Flags    = Pass->GetFlags();
        DebugPass.bEnabled = Pass->IsEnabled();
        DebugPass.bCulled  = Pass->IsCulled();

        // A pass that never runs must not shadow the pass that genuinely produced the resource.
        const bool bTracksWrites = DebugPass.bEnabled && !DebugPass.bCulled;

        auto AddAccess = [&](int32 ResourceIndex, ERHIResourceState State, bool bIsWrite, bool bWriteLoadsPrevious)
        {
            if (ResourceIndex < 0)
            {
                return;
            }

            const int32 AccessIndex = DebugPass.Accesses.Size();
            FRenderGraphDebugAccess& Access = DebugPass.Accesses.Emplace();
            Access.ResourceIndex = ResourceIndex;
            Access.State         = State;
            Access.bIsWrite      = bIsWrite;

            // Reads always depend on the last writer, writes only when they build on what it left behind.
            const TPair<int32, int32>* Writer = LastWriterByResource.Find(ResourceIndex);
            if (Writer && Writer->First != PassIndex && (!bIsWrite || bWriteLoadsPrevious))
            {
                FRenderGraphDebugLink& Link = Out.Links.Emplace();
                Link.FromPass      = Writer->First;
                Link.FromAccess    = Writer->Second;
                Link.ToPass        = PassIndex;
                Link.ToAccess      = AccessIndex;
                Link.ResourceIndex = ResourceIndex;
            }

            if (bIsWrite && bTracksWrites)
            {
                LastWriterByResource.Add(ResourceIndex, TPair<int32, int32>(PassIndex, AccessIndex));
            }
        };

        for (const FRenderGraphTextureAccess& Access : Pass->GetTextureAccesses())
        {
            int32 ResourceIndex = -1;
            if (Access.Resource)
            {
                if (const int32* Found = TextureIndices.Find(Access.Resource))
                {
                    ResourceIndex = *Found;
                }
            }

            AddAccess(ResourceIndex, Access.State, Access.bIsWrite, WriteLoadsPreviousContents(*Pass, Access.Resource));
        }

        for (const FRenderGraphBufferAccess& Access : Pass->GetBufferAccesses())
        {
            int32 ResourceIndex = -1;
            if (Access.Resource)
            {
                if (const int32* Found = BufferIndices.Find(Access.Resource))
                {
                    ResourceIndex = *Found;
                }
            }

            AddAccess(ResourceIndex, Access.State, Access.bIsWrite, true);
        }
    }
}
