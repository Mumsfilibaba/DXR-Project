#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Threading/Atomic/AtomicBool.h"
#include "Core/Threading/Spinlock.h"
#include "RendererCore/RenderGraph/RenderGraphBuilder.h"

enum class ERenderGraphDebugResourceKind : uint8
{
    Texture,
    Buffer,
};

struct FRenderGraphDebugResource
{
    String                        Name;
    ERenderGraphDebugResourceKind Kind        = ERenderGraphDebugResourceKind::Texture;
    bool                          bIsExternal = false;
    uint32                        Width       = 0;
    uint32                        Height      = 0;
    uint32                        DepthOrSize = 0;
};

struct FRenderGraphDebugAccess
{
    int32             ResourceIndex = -1;
    ERHIResourceState State         = ERHIResourceState::Common;
    bool              bIsWrite      = false;
};

struct FRenderGraphDebugPass
{
    String                          Name;
    ERenderGraphPassFlags           Flags    = ERenderGraphPassFlags::None;
    bool                            bEnabled = true;
    bool                            bCulled  = false;
    TArray<FRenderGraphDebugAccess> Accesses;
};

struct FRenderGraphDebugLink
{
    int32 FromPass      = -1;
    int32 FromAccess    = -1;
    int32 ToPass        = -1;
    int32 ToAccess      = -1;
    int32 ResourceIndex = -1;
};

struct FRenderGraphDebugSnapshot
{
    String                            GraphName;
    FRenderGraphStatistics            Statistics;
    TArray<FRenderGraphDebugResource> Resources;
    TArray<FRenderGraphDebugPass>     Passes;
    TArray<FRenderGraphDebugLink>     Links;
};

struct RENDERERCORE_API RenderGraphDebug
{
    static void SetCaptureEnabled(bool bEnabled);
    static bool IsCaptureEnabled();

    static void CaptureSnapshot(const FRenderGraphBuilder& Builder, FRenderGraphDebugSnapshot& Out);

    static void Publish(FRenderGraphDebugSnapshot&& Snapshot);
    static bool CopyLatest(FRenderGraphDebugSnapshot& Out);

private:
    static AtomicBool                bCaptureEnabled;
    static FSpinLock                 SnapshotLock;
    static FRenderGraphDebugSnapshot LatestSnapshot;
    static bool                      bHasSnapshot;
};
