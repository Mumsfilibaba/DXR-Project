#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalCapabilities.h"
#include "MetalRHI/MetalDeviceDebug.h"
#include "MetalRHI/MetalAllocators.h"
#include "MetalRHI/MetalBinaryArchive.h"
#include "MetalRHI/MetalBindlessDescriptors.h"
#include "MetalRHI/MetalCommandContext.h"
#include "MetalRHI/MetalPipelineCache.h"
#include "MetalRHI/MetalQueue.h"
#include "MetalRHI/MetalRelocatable.h"
#include "MetalRHI/MetalResidencyManager.h"
#include "MetalRHI/MetalResidencySet.h"
#include "MetalRHI/MetalShaderLibraryCache.h"
#include "MetalRHI/MetalStats.h"
#include "Core/Math/Math.h"
#include "Core/Memory/Memory.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/CoreDelegates.h"
#include "Core/Misc/Paths.h"
#include "Core/Platform/PlatformTime.h"
#include "Core/Threading/ScopedLock.h"
#include <CoreGraphics/CoreGraphics.h>

static TAutoConsoleVariable<bool> CVarEnableMeshShaders(
    "MetalRHI.EnableMeshShaders",
    "Enables mesh and amplification shaders on Metal 3 GPUs. Disable to work around a driver that misbehaves on the SPIRV-Cross mesh output",
    true);

static TAutoConsoleVariable<bool> CVarForceMetal2Features(
    "MetalRHI.ForceMetal2Features",
    "Masks the device down to a Metal 2 feature set (no Metal 3, Tier 1 argument buffers, no mesh shaders, no vertex amplification) to exercise the non-bindless paths",
    false);

static TAutoConsoleVariable<String> CVarPreferredDeviceName(
    "MetalRHI.PreferredDeviceName",
    "Selects the Metal device whose name contains this text, ignoring the device scoring when it matches",
    "");

static TAutoConsoleVariable<bool> CVarPreferDisplayDevice(
    "MetalRHI.PreferDisplayDevice",
    "Renders on the GPU that drives the main display, which avoids copying every presented frame between GPUs. MetalRHI.PreferredDeviceName still wins when it matches",
    false);

static TAutoConsoleVariable<float> CVarLogMemoryStatsInterval(
    "MetalRHI.LogMemoryStatsInterval",
    "Logs a summary of Metal memory usage every this many seconds, or never when zero",
    0.0f);

static TAutoConsoleVariable<String> CVarDynamicConstantsStorage(
    "MetalRHI.DynamicConstantsStorage",
    "Storage mode of the dynamic constant pages: Auto (Managed on discrete GPUs, Shared otherwise), Shared or Managed",
    "Auto");

static TAutoConsoleVariable<int32> CVarMaxDefragMovesPerFrame(
    "MetalRHI.MaxDefragMovesPerFrame",
    "Most heap-placed resources copied out of the sparsest heap each frame, which the next frame waits for before swapping them in. Zero disables defragmentation",
    4);

static TAutoConsoleVariable<int32> CVarDefragEligibilityDelay(
    "MetalRHI.DefragEligibilityDelay",
    "Frames an allocation has to live before a defrag move may relocate it, so resources created and filled this frame are left alone",
    1);

static TAutoConsoleVariable<String> CVarBinaryArchiveMode(
    "MetalRHI.BinaryArchiveMode",
    "How the MTLBinaryArchive is used: Ignore, Use (load read-only), Append (load, then save every pipeline this session created) or Create (start empty, then save every pipeline this session created)",
    "Append");

static TAutoConsoleVariable<String> CVarBinaryArchiveFileName(
    "MetalRHI.BinaryArchiveFileName",
    "File name of the MTLBinaryArchive in the asset directory. A .version sidecar sits beside it",
    "PipelineCache.metalarchive");

static constexpr MTLResourceOptions GNullResourceOptions = MTLResourceStorageModePrivate | MTLResourceHazardTrackingModeUntracked;
static constexpr NSUInteger         GNullSampleCount     = 4;

static MTLResourceOptions GetDynamicConstantsOptions()
{
    const String Storage = CVarDynamicConstantsStorage.GetValue();

    if (Storage.Equals("Shared", EStringCaseType::NoCase))
    {
        return MetalRHI::GetMTLResourceOptions(EMetalMemoryClass::Upload);
    }

    if (Storage.Equals("Managed", EStringCaseType::NoCase))
    {
        return MTLResourceStorageModeManaged | MTLResourceCPUCacheModeWriteCombined | MTLResourceHazardTrackingModeUntracked;
    }

    return MetalRHI::GetMTLResourceOptions(EMetalMemoryClass::CPUWriteGPURead);
}

static MTLTextureType GetNullMTLTextureType(EMSLTextureDimension Dimension)
{
    switch (Dimension)
    {
        case EMSLTextureDimension::Texture1D:        return MTLTextureType1D;
        case EMSLTextureDimension::Texture1DArray:   return MTLTextureType1DArray;
        case EMSLTextureDimension::Texture2D:        return MTLTextureType2D;
        case EMSLTextureDimension::Texture2DArray:   return MTLTextureType2DArray;
        case EMSLTextureDimension::TextureCube:      return MTLTextureTypeCube;
        case EMSLTextureDimension::TextureCubeArray: return MTLTextureTypeCubeArray;
        case EMSLTextureDimension::Texture3D:        return MTLTextureType3D;
        case EMSLTextureDimension::Texture2DMS:      return MTLTextureType2DMultisample;
        case EMSLTextureDimension::TextureBuffer:    return MTLTextureTypeTextureBuffer;
        default:                                     return MTLTextureType2D;
    }
}

static MTLPixelFormat GetNullPixelFormat(EMSLTextureComponent Component, bool bWritable)
{
    const bool bRGBA8 = !bWritable || GMetalReadWriteTextureTier >= MTLReadWriteTextureTier2;
    switch (Component)
    {
        case EMSLTextureComponent::Int:   return bRGBA8 ? MTLPixelFormatRGBA8Sint  : MTLPixelFormatR32Sint;
        case EMSLTextureComponent::Uint:  return bRGBA8 ? MTLPixelFormatRGBA8Uint  : MTLPixelFormatR32Uint;
        case EMSLTextureComponent::Depth: return MTLPixelFormatDepth32Float;
        default:                          return bRGBA8 ? MTLPixelFormatRGBA8Unorm : MTLPixelFormatR32Float;
    }
}

static bool HasNullTexture(EMSLTextureDimension Dimension, EMSLTextureComponent Component, bool bWritable)
{
    const bool bIsCube = Dimension == EMSLTextureDimension::TextureCube || Dimension == EMSLTextureDimension::TextureCubeArray;

    if (Component == EMSLTextureComponent::Depth)
    {
        return !bWritable && (bIsCube || Dimension == EMSLTextureDimension::Texture2D || Dimension == EMSLTextureDimension::Texture2DArray || Dimension == EMSLTextureDimension::Texture2DMS);
    }

    return !bWritable || (!bIsCube && Dimension != EMSLTextureDimension::Texture2DMS);
}

static uint8 GetNullTextureFallback(EMSLTextureDimension Dimension, EMSLTextureComponent Component, bool bWritable)
{
    if (Component == EMSLTextureComponent::Depth)
    {
        Component = EMSLTextureComponent::Float;
    }

    if (bWritable)
    {
        if (Dimension == EMSLTextureDimension::TextureCube || Dimension == EMSLTextureDimension::TextureCubeArray)
        {
            Dimension = EMSLTextureDimension::Texture2DArray;
        }
        else if (Dimension == EMSLTextureDimension::Texture2DMS)
        {
            Dimension = EMSLTextureDimension::Texture2D;
        }
    }

    return MakeMSLNullTextureType(Dimension, Component);
}

static void EncodeZeroFill(id<MTLBlitCommandEncoder> Encoder, id<MTLBuffer> Zeros, id<MTLTexture> Texture)
{
    const MTLTextureType TextureType  = Texture.textureType;
    const bool           bIsTexture1D = TextureType == MTLTextureType1D || TextureType == MTLTextureType1DArray;
    const bool           bIsCube      = TextureType == MTLTextureTypeCube || TextureType == MTLTextureTypeCubeArray;

    const NSUInteger BytesPerRow   = bIsTexture1D ? 0 : 4;
    const NSUInteger BytesPerImage = TextureType == MTLTextureType3D ? 4 : 0;

    for (NSUInteger Slice = 0; Slice < (bIsCube ? 6 : 1); ++Slice)
    {
        [Encoder copyFromBuffer:Zeros
                   sourceOffset:0
              sourceBytesPerRow:BytesPerRow
            sourceBytesPerImage:BytesPerImage
                     sourceSize:MTLSizeMake(1, 1, 1)
                      toTexture:Texture
               destinationSlice:Slice
               destinationLevel:0
              destinationOrigin:MTLOriginMake(0, 0, 0)];
    }
}

static void EncodeClearPasses(id<MTLCommandBuffer> CommandBuffer, id<MTLTexture> Texture)
{
    const bool bIsDepth = Texture.pixelFormat == MTLPixelFormatDepth32Float;
    const bool bIsCube  = Texture.textureType == MTLTextureTypeCube || Texture.textureType == MTLTextureTypeCubeArray;

    for (NSUInteger Slice = 0; Slice < (bIsCube ? 6 : 1); ++Slice)
    {
        MTLRenderPassDescriptor*           Descriptor = [MTLRenderPassDescriptor renderPassDescriptor];
        MTLRenderPassAttachmentDescriptor* Attachment = bIsDepth
            ? static_cast<MTLRenderPassAttachmentDescriptor*>(Descriptor.depthAttachment)
            : Descriptor.colorAttachments[0];
        Attachment.texture     = Texture;
        Attachment.slice       = Slice;
        Attachment.loadAction  = MTLLoadActionClear;
        Attachment.storeAction = MTLStoreActionStore;

        if (bIsDepth)
        {
            Descriptor.depthAttachment.clearDepth = 0.0;
        }
        else
        {
            Descriptor.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 0.0);
        }

        [[CommandBuffer renderCommandEncoderWithDescriptor:Descriptor] endEncoding];
    }
}

FMetalDevice::FMetalDevice()
    : ResidencySet(nullptr)
    , ResidencyManager(nullptr)
    , ShaderLibraryCache(nullptr)
    , PipelineCache(nullptr)
    , BinaryArchive(nullptr)
    , BindlessDescriptorManager(nullptr)
    , StagingBufferAllocator(nullptr)
    , DynamicConstantsAllocator(nullptr)
    , BufferAllocator(nullptr)
    , TextureAllocator(nullptr)
    , UploadHeapAllocator(nullptr)
    , Queues()
    , TimestampQueries(this)
    , OcclusionQueries(this)
    , StatisticQueries(this)
    , StageUtilizationQueries(this)
    , DefaultResources{}
    , DepthStencilStates()
    , DepthStencilStatesCS()
    , LatestUploadValue(0)
    , PendingDefragMoves()
    , DefragWaitValues()
    , DefragCS()
    , bHasPendingDefragMoves(false)
    , FrameCounter(0)
    , SharedBytes(0)
    , ManagedBytes(0)
    , LastMemoryLogTime(0)
    , Properties{}
    , Device(nil)
    , DeviceObserver(nil)
    , bDeviceRemoved(false)
{
    Queues.Fill(nullptr);
    DefragWaitValues.Fill(0);
}

FMetalDevice::~FMetalDevice()
{
    if (DeviceObserver)
    {
        MTLRemoveDeviceObserver(DeviceObserver);
        [DeviceObserver release];
        DeviceObserver = nil;
    }

    WaitForGPU();

    bHasPendingDefragMoves.Store(!PendingDefragMoves.IsEmpty());
    FinalizeDefragMoves();

    if (BinaryArchive)
    {
        BinaryArchive->WaitForSave();
        BinaryArchive->Save();
    }

    SAFE_DELETE(PipelineCache);
    SAFE_DELETE(ShaderLibraryCache);
    SAFE_DELETE(BinaryArchive);

    if (ResidencySet && DefaultResources.Heap)
    {
        ResidencySet->RemoveHeap(DefaultResources.Heap);
    }

    for (id<MTLTexture> Texture : DefaultResources.OwnedTextures)
    {
        [Texture release];
    }

    [DefaultResources.NullBuffer release];
    [DefaultResources.Heap release];
    [DefaultResources.DefaultSampler release];

    for (const auto Pair : DepthStencilStates)
    {
        [Pair.Second release];
    }

    DepthStencilStates.Clear();

    if (StagingBufferAllocator)
    {
        StagingBufferAllocator->CleanUp();
    }

    if (DynamicConstantsAllocator)
    {
        DynamicConstantsAllocator->CleanUp();
    }

    if (UploadHeapAllocator)
    {
        UploadHeapAllocator->CleanUp();
    }

    if (BufferAllocator)
    {
        BufferAllocator->CleanUp();
    }

    if (TextureAllocator)
    {
        TextureAllocator->CleanUp();
    }

    SAFE_DELETE(TextureAllocator);
    SAFE_DELETE(BufferAllocator);
    SAFE_DELETE(BindlessDescriptorManager);
    SAFE_DELETE(UploadHeapAllocator);
    SAFE_DELETE(DynamicConstantsAllocator);
    SAFE_DELETE(StagingBufferAllocator);
    SAFE_DELETE(ResidencyManager);

    if (ResidencySet)
    {
        ResidencySet->Release();
        SAFE_DELETE(ResidencySet);
    }

    for (int32 Index = Queues.Size() - 1; Index >= 0; --Index)
    {
        SAFE_DELETE(Queues[Index]);
    }

    [Device release];
    Device = nil;
}

void FMetalDevice::BeginFrame()
{
    const uint64 Frame = FrameCounter.Increment();
    MetalBeginFrameCapture(Device);

    ForEachQueue([Frame](FMetalQueue& Queue)
    {
        Queue.PruneCommandContexts(Frame);
    });

    ProcessQueues();
}

void FMetalDevice::EndFrame()
{
    if (DynamicConstantsAllocator)
    {
        DynamicConstantsAllocator->EndFrame();
    }

    if (UploadHeapAllocator)
    {
        UploadHeapAllocator->EndFrame();
    }

    ProcessQueues();

    if (ResidencyManager)
    {
        ResidencyManager->EndFrame();
        ResidencyManager->EvictIfNeeded();
    }

    PipelineCache->Prune();
    ShaderLibraryCache->Prune();
    BinaryArchive->SaveAsync();

    MetalEndFrameCapture();

#if METAL_ENABLE_STATS
    LogMemoryStats();
#endif
}

uint32 FMetalDevice::RecordDefragMoves(FMetalCommandContext& Context)
{
    const int32 MaxMoves = CVarMaxDefragMovesPerFrame.GetValue();

    if (MaxMoves <= 0 || !BufferAllocator || !TextureAllocator)
    {
        return 0;
    }

    TScopedLock Lock(DefragCS);

    if (!PendingDefragMoves.IsEmpty())
    {
        return 0;
    }

    const uint64 Delay               = static_cast<uint64>(Math::Max(CVarDefragEligibilityDelay.GetValue(), 0));
    const uint64 NextFrame           = GetFrameCounter() + 1;
    const uint64 EligibleBeforeFrame = NextFrame > Delay ? NextFrame - Delay : 0;

    // The copies read what earlier encoders wrote, and the encoder fence only orders encoders opened after this point
    FMetalEncoderManager& Encoders = Context.GetEncoders();
    Encoders.EndEncoder();

    uint32 NumMoves = TextureAllocator->RecordDefragMoves(Context, static_cast<uint32>(MaxMoves), EligibleBeforeFrame, PendingDefragMoves);
    NumMoves += BufferAllocator->RecordDefragMoves(Context, static_cast<uint32>(MaxMoves) - NumMoves, EligibleBeforeFrame, PendingDefragMoves);

    if (NumMoves > 0)
    {
        Encoders.EndEncoder();
    }

    STAT_SET(STAT_Metal_DefragPending, NumMoves);
    return NumMoves;
}

void FMetalDevice::SetDefragWaitValues(uint64 DirectValue)
{
    TScopedLock Lock(DefragCS);

    ForEachQueue([this](FMetalQueue& Queue)
    {
        DefragWaitValues[static_cast<uint32>(Queue.GetType())] = Queue.GetLastSubmittedValue();
    });

    uint64& DirectWaitValue = DefragWaitValues[static_cast<uint32>(EMetalQueueType::Direct)];
    DirectWaitValue = Math::Max(DirectWaitValue, DirectValue);

    // Raised only now, so a context started before the copies were submitted cannot swap ahead of them
    bHasPendingDefragMoves.Store(!PendingDefragMoves.IsEmpty());
}

void FMetalDevice::FinalizeDefragMoves()
{
    if (!bHasPendingDefragMoves.Load())
    {
        return;
    }

    // Held across the swaps, so a resource released on another thread either cancels first or releases the swapped placement
    TScopedLock Lock(DefragCS);

    if (!bHasPendingDefragMoves.Load() || PendingDefragMoves.IsEmpty())
    {
        return;
    }

    ForEachQueue([this](FMetalQueue& Queue)
    {
        const uint64 Value = DefragWaitValues[static_cast<uint32>(Queue.GetType())];

        if (Value != 0)
        {
            Queue.WaitForValue(Value);
        }
    });

    for (FMetalDefragMove& Move : PendingDefragMoves)
    {
        if (!Move.bCancelled)
        {
            Move.Storage->GetOwner()->Relocate(*Move.Target);
            STAT_ADD(STAT_Metal_DefragMoves, 1);
        }

        Move.Target->ReleaseResource();
    }

    PendingDefragMoves.Clear();
    DefragWaitValues.Fill(0);
    bHasPendingDefragMoves.Store(false);
    STAT_SET(STAT_Metal_DefragPending, 0);
}

void FMetalDevice::CancelDefragMove(FMetalResourceStorage& Storage)
{
    TScopedLock Lock(DefragCS);

    for (FMetalDefragMove& Move : PendingDefragMoves)
    {
        if (Move.Storage == &Storage && !Move.bCancelled)
        {
            Move.bCancelled = true;
            Storage.SetDefragPending(false);
            STAT_ADD(STAT_Metal_DefragCancels, 1);
            return;
        }
    }
}

void FMetalDevice::TrimAllocatorCaches()
{
    if (BufferAllocator)
    {
        BufferAllocator->Trim();
    }

    if (TextureAllocator)
    {
        TextureAllocator->Trim();
    }

    if (StagingBufferAllocator)
    {
        StagingBufferAllocator->Trim();
    }

    if (DynamicConstantsAllocator)
    {
        DynamicConstantsAllocator->Trim();
    }

    if (UploadHeapAllocator)
    {
        UploadHeapAllocator->Trim();
    }
}

bool FMetalDevice::Initialize()
{
    if (!CreateDevice())
    {
        return false;
    }

    QueryDeviceFeatureSupport();

    if (!CreatePipelineCaches())
    {
        return false;
    }

    if (!CreateCommandQueues())
    {
        return false;
    }

    ResidencySet = new FMetalResidencySet(this);
    ResidencySet->Initialize();
    GMetalFeatures.bResidencySets = !ResidencySet->UsesEncoderFallback();

    ResidencyManager = new FMetalResidencyManager(this);

    StagingBufferAllocator    = new FMetalLinearAllocator(this, 2ull * 1024ull * 1024ull, 2ull * 1024ull * 1024ull, MetalRHI::GetMTLResourceOptions(EMetalMemoryClass::Upload), false, EMetalAllocationLifetime::Submission);
    DynamicConstantsAllocator = new FMetalLinearAllocator(this, 4ull * 1024ull * 1024ull, 2ull * 1024ull * 1024ull, GetDynamicConstantsOptions(), true, EMetalAllocationLifetime::Frame);
    UploadHeapAllocator       = new FMetalUploadHeapAllocator(this, 2ull * 1024ull * 1024ull, 2ull * 1024ull * 1024ull);
    BufferAllocator           = new FMetalBufferAllocator(this);
    TextureAllocator          = new FMetalTextureAllocator(this);

    if (!CreateDefaultResources())
    {
        METAL_ERROR("Failed to initialize default Metal resources");
        return false;
    }

    ResidencySet->AddHeap(DefaultResources.Heap);

    BindlessDescriptorManager = new FMetalBindlessDescriptorManager(this);
    GMetalSupportsBindless = BindlessDescriptorManager->Initialize();

    if (!GMetalSupportsBindless)
    {
        SAFE_DELETE(BindlessDescriptorManager);
    }

    GMetalSupportsTimestampQueries = GMetalSupportsCounterSampling && TimestampQueries.Initialize();

    if (!GMetalSupportsTimestampQueries)
    {
        TimestampQueries.Release();
        METAL_INFO("Timestamp queries are unavailable on this Metal device");
    }
    else if (!MetalRHI::SamplesTimestampsAtStageBoundary())
    {
        METAL_INFO("Timestamp samples are quantized to encoder boundaries");
    }

    GMetalSupportsStatisticQueries = GMetalSupportsCounterSampling && StatisticQueries.Initialize();

    if (!GMetalSupportsStatisticQueries)
    {
        StatisticQueries.Release();
        METAL_INFO("Pipeline statistics queries are unavailable on this Metal device");
    }

    if (GMetalSupportsCounterSampling && StageUtilizationQueries.Initialize())
    {
        METAL_INFO("Stage utilization stats sample every render and compute encoder");
    }

    if (!OcclusionQueries.Initialize())
    {
        METAL_ERROR("Failed to initialize occlusion queries");
        return false;
    }

    return true;
}

bool FMetalDevice::CreateDevice()
{
    Device = SelectDevice();

    if (!Device)
    {
        METAL_ERROR("Failed to select a Metal device");
        return false;
    }

    ReadDeviceProperties();

#if METAL_ASSUME_APPLE_GPU
    CHECK([Device supportsFamily:MTLGPUFamilyApple7]);
#endif

    return true;
}

bool FMetalDevice::CreatePipelineCaches()
{
    ShaderLibraryCache = new FMetalShaderLibraryCache(this);
    BinaryArchive      = new FMetalBinaryArchive(this);
    PipelineCache      = new FMetalPipelineCache(this);

    const String ArchivePath = Paths::GetAssetDir() + '/' + CVarBinaryArchiveFileName.GetValue();
    return BinaryArchive->Initialize(FMetalBinaryArchive::ParseMode(CVarBinaryArchiveMode.GetValue()), ArchivePath);
}

int32 FMetalDevice::ScoreDevice(id<MTLDevice> CandidateDevice)
{
    if (!CandidateDevice)
    {
        return -1;
    }

    int32 Score = 0;

    if (!CandidateDevice.isLowPower)
    {
        Score += 100;
    }

    if (!CandidateDevice.isHeadless)
    {
        Score += 25;
    }

    // An external GPU is the faster device often enough that being removable only breaks a tie.
    if (!CandidateDevice.isRemovable)
    {
        Score += 5;
    }

    if (CandidateDevice.recommendedMaxWorkingSetSize > 0)
    {
        Score += static_cast<int32>(CandidateDevice.recommendedMaxWorkingSetSize / (256ull * 1024ull * 1024ull));
    }

    return Score;
}

id<MTLDevice> FMetalDevice::SelectDevice()
{
    SCOPED_AUTORELEASE_POOL();

    id<NSObject>            Observer         = nil;
    NSArray<id<MTLDevice>>* AvailableDevices = MTLCopyAllDevicesWithObserver(&Observer, ^(id<MTLDevice> NotifiedDevice, MTLDeviceNotificationName Notification)
    {
        const bool bRemoval = [Notification isEqualToString:MTLDeviceRemovalRequestedNotification] || [Notification isEqualToString:MTLDeviceWasRemovedNotification];

        if (!bRemoval || NotifiedDevice != Device || bDeviceRemoved.Exchange(true))
        {
            return;
        }

        METAL_ERROR("Metal device '%s' is being removed, rendering cannot continue", *Properties.Name);
        CoreDelegates::DeviceRemovedDelegate.Broadcast();
    });

    DeviceObserver = [Observer retain];

    const String PreferredName = CVarPreferredDeviceName.GetValue();

    id<MTLDevice> SelectedDevice = nil;
    id<MTLDevice> PreferredMatch = nil;
    int32         BestScore      = -1;

    for (id<MTLDevice> CandidateDevice in AvailableDevices)
    {
        const String Name(CandidateDevice.name);
        const int32  Score = ScoreDevice(CandidateDevice);

        METAL_INFO("Metal device candidate '%s' (score=%d, %llu MB, low-power=%s, removable=%s, headless=%s)",
            *Name,
            Score,
            CandidateDevice.recommendedMaxWorkingSetSize / (1024ull * 1024ull),
            CandidateDevice.isLowPower  ? "yes" : "no",
            CandidateDevice.isRemovable ? "yes" : "no",
            CandidateDevice.isHeadless  ? "yes" : "no");

        if (!PreferredMatch && !PreferredName.IsEmpty() && Name.Contains(PreferredName, EStringCaseType::NoCase))
        {
            PreferredMatch = CandidateDevice;
        }

        if (Score > BestScore)
        {
            BestScore      = Score;
            SelectedDevice = CandidateDevice;
        }
    }

    if (!PreferredName.IsEmpty())
    {
        if (PreferredMatch)
        {
            SelectedDevice = PreferredMatch;
        }
        else
        {
            METAL_WARNING("No Metal device matches MetalRHI.PreferredDeviceName='%s', falling back to the highest scoring device", *PreferredName);
        }
    }

    id<MTLDevice> DisplayDevice = [CGDirectDisplayCopyCurrentMetalDevice(CGMainDisplayID()) autorelease];

    if (DisplayDevice)
    {
        const String DisplayName(DisplayDevice.name);
        METAL_INFO("The main display is driven by '%s'", *DisplayName);

        if (CVarPreferDisplayDevice.GetValue() && !PreferredMatch)
        {
            SelectedDevice = DisplayDevice;
        }
        else if (SelectedDevice && SelectedDevice.registryID != DisplayDevice.registryID)
        {
            const String SelectedName(SelectedDevice.name);
            METAL_WARNING("Rendering on '%s' while '%s' drives the main display, so every presented frame is copied between the GPUs. "
                "Set MetalRHI.PreferDisplayDevice to render on the display GPU", *SelectedName, *DisplayName);
        }
    }

    if (SelectedDevice)
    {
        [SelectedDevice retain];
    }

    [AvailableDevices release];

    if (!SelectedDevice)
    {
        SelectedDevice = MTLCreateSystemDefaultDevice();
    }

    return SelectedDevice;
}

void FMetalDevice::ReadDeviceProperties()
{
    Properties.Name                         = Device.name;
    Properties.RegistryID                   = Device.registryID;
    Properties.RecommendedMaxWorkingSetSize = Device.recommendedMaxWorkingSetSize;
    Properties.MaxBufferLength              = static_cast<uint64>(Device.maxBufferLength);
    Properties.MaxThreadsPerThreadgroup     = Device.maxThreadsPerThreadgroup;
    Properties.ArgumentBuffersTier          = Device.argumentBuffersSupport;
    Properties.bHasUnifiedMemory            = Device.hasUnifiedMemory;
    Properties.bIsLowPower                  = Device.isLowPower;
    Properties.bIsRemovable                 = Device.isRemovable;
    Properties.bIsHeadless                  = Device.isHeadless;
    Properties.HighestSupportedFamily       = MTLGPUFamilyApple1;

    static const MTLGPUFamily Families[] =
    {
        MTLGPUFamilyApple9,
        MTLGPUFamilyApple8,
        MTLGPUFamilyApple7,
        MTLGPUFamilyApple6,
        MTLGPUFamilyApple5,
        MTLGPUFamilyApple4,
        MTLGPUFamilyApple3,
        MTLGPUFamilyApple2,
        MTLGPUFamilyApple1,
        MTLGPUFamilyMac2,
        MTLGPUFamilyCommon3,
        MTLGPUFamilyCommon2,
        MTLGPUFamilyCommon1,
    };

    for (MTLGPUFamily Family : Families)
    {
        if ([Device supportsFamily:Family])
        {
            Properties.HighestSupportedFamily = Family;
            break;
        }
    }

    const String DeviceClass([[Device class] description]);
    METAL_INFO("Selected Device=%s (%s)", *Properties.Name, *DeviceClass);

    if (DeviceClass.Contains("Capture"))
    {
        METAL_WARNING("Metal GPU capture is wrapping the device, which inflates the reported memory usage. Disable GPU Frame Capture in the scheme, or launch the app outside Xcode, before measuring memory");
    }

#if METAL_ENABLE_DEBUG_LAYER
    if (DeviceClass.Contains("MTLDebug"))
    {
        METAL_INFO("Metal debug layer is live, the selected device is a debug wrapper");
    }
    else if (DeviceClass.Contains("Capture"))
    {
        METAL_INFO("The debug layer sits underneath the capture wrapper and cannot be read from the device class");
    }
    else if (MetalIsDebugLayerRequested())
    {
        METAL_WARNING("Metal debug layer was requested but is not wrapping the device, something created an MTLDevice before the environment was latched");
    }
#endif
}

bool FMetalDevice::CreateCommandQueues()
{
    for (uint32 Index = 0; Index < static_cast<uint32>(EMetalQueueType::Count); ++Index)
    {
        Queues[Index] = new FMetalQueue(this, static_cast<EMetalQueueType>(Index));

        if (!Queues[Index]->Initialize())
        {
            METAL_ERROR("Failed to initialize Metal queue %u", Index);
            return false;
        }
    }

    return true;
}

bool FMetalDevice::CreateDefaultResources()
{
    SCOPED_AUTORELEASE_POOL();

    struct FNullTextureDesc
    {
        MTLTextureDescriptor* Descriptor;
        uint8                 NullTextureType;
        bool                  bWritable;
    };

    TArray<FNullTextureDesc> HeapTextures;
    TArray<FNullTextureDesc> BufferTextures;

    for (uint8 Component = 0; Component < static_cast<uint8>(EMSLTextureComponent::Count); ++Component)
    {
        for (uint8 Dimension = 0; Dimension < static_cast<uint8>(EMSLTextureDimension::Count); ++Dimension)
        {
            for (const bool bWritable : { false, true })
            {
                const EMSLTextureDimension TextureDimension = static_cast<EMSLTextureDimension>(Dimension);
                const EMSLTextureComponent TextureComponent = static_cast<EMSLTextureComponent>(Component);

                if (!HasNullTexture(TextureDimension, TextureComponent, bWritable))
                {
                    continue;
                }

                const MTLPixelFormat Format        = GetNullPixelFormat(TextureComponent, bWritable);
                const bool           bMultisampled = TextureDimension == EMSLTextureDimension::Texture2DMS;
                const bool           bNeedsClear   = bMultisampled || TextureComponent == EMSLTextureComponent::Depth;

                MTLTextureUsage Usage = MTLTextureUsageShaderRead;

                if (bWritable)
                {
                    Usage |= MTLTextureUsageShaderWrite;
                }

                if (bNeedsClear)
                {
                    Usage |= MTLTextureUsageRenderTarget;
                }

                const uint8 NullTextureType = MakeMSLNullTextureType(TextureDimension, TextureComponent);

                if (TextureDimension == EMSLTextureDimension::TextureBuffer)
                {
                    MTLTextureDescriptor* Descriptor = [MTLTextureDescriptor textureBufferDescriptorWithPixelFormat:Format width:1 resourceOptions:GNullResourceOptions usage:Usage];
                    BufferTextures.Add(FNullTextureDesc{ Descriptor, NullTextureType, bWritable });
                    continue;
                }

                MTLTextureDescriptor* Descriptor = [[MTLTextureDescriptor new] autorelease];
                Descriptor.textureType        = GetNullMTLTextureType(TextureDimension);
                Descriptor.pixelFormat        = Format;
                Descriptor.width              = 1;
                Descriptor.height             = 1;
                Descriptor.depth              = 1;
                Descriptor.mipmapLevelCount   = 1;
                Descriptor.sampleCount        = bMultisampled ? GNullSampleCount : 1;
                Descriptor.arrayLength        = 1;
                Descriptor.usage              = Usage;
                Descriptor.resourceOptions    = GNullResourceOptions;
                HeapTextures.Add(FNullTextureDesc{ Descriptor, NullTextureType, bWritable });
            }
        }
    }

    const auto AlignUp = [](NSUInteger Value, NSUInteger Alignment)
    {
        return (Value + Alignment - 1) & ~(Alignment - 1);
    };

    const MTLSizeAndAlign BufferSizeAndAlign = [Device heapBufferSizeAndAlignWithLength:FMetalDefaultResources::NullBufferSize options:GNullResourceOptions];
    NSUInteger            HeapSize           = BufferSizeAndAlign.size;
    for (const FNullTextureDesc& Desc : HeapTextures)
    {
        const MTLSizeAndAlign SizeAndAlign = [Device heapTextureSizeAndAlignWithDescriptor:Desc.Descriptor];
        HeapSize = AlignUp(HeapSize, SizeAndAlign.align) + AlignUp(SizeAndAlign.size, SizeAndAlign.align);
    }

    MTLHeapDescriptor* HeapDescriptor = [[MTLHeapDescriptor new] autorelease];
    HeapDescriptor.type               = MTLHeapTypeAutomatic;
    HeapDescriptor.storageMode        = MTLStorageModePrivate;
    HeapDescriptor.hazardTrackingMode = MTLHazardTrackingModeUntracked;
    HeapDescriptor.size               = HeapSize;

    DefaultResources.Heap = [Device newHeapWithDescriptor:HeapDescriptor];

    if (!DefaultResources.Heap)
    {
        METAL_ERROR("Failed to create the %llu byte heap for the default resources", static_cast<uint64>(HeapSize));
        return false;
    }

    DefaultResources.Heap.label = @"MetalRHI.DefaultResources";

    DefaultResources.NullBuffer = [DefaultResources.Heap newBufferWithLength:FMetalDefaultResources::NullBufferSize options:GNullResourceOptions];

    if (!DefaultResources.NullBuffer)
    {
        METAL_ERROR("Failed to create the null buffer");
        return false;
    }

    DefaultResources.NullBuffer.label = @"MetalRHI.NullBuffer";

    Memory::Memzero(DefaultResources.NullTextures, sizeof(DefaultResources.NullTextures));
    Memory::Memzero(DefaultResources.NullRWTextures, sizeof(DefaultResources.NullRWTextures));

    const auto CreateTextures = [&](const TArray<FNullTextureDesc>& Descs, bool bFromBuffer) -> bool
    {
        for (const FNullTextureDesc& Desc : Descs)
        {
            id<MTLTexture> Texture = nil;

            if (bFromBuffer)
            {
                const NSUInteger BytesPerRow = AlignUp(4, [Device minimumTextureBufferAlignmentForPixelFormat:Desc.Descriptor.pixelFormat]);
                Texture = [DefaultResources.NullBuffer newTextureWithDescriptor:Desc.Descriptor offset:0 bytesPerRow:BytesPerRow];
            }
            else
            {
                Texture = [DefaultResources.Heap newTextureWithDescriptor:Desc.Descriptor];
            }

            if (!Texture)
            {
                METAL_ERROR("Failed to create null %s texture %u", Desc.bWritable ? "writable" : "read-only", Desc.NullTextureType);
                return false;
            }

            DefaultResources.OwnedTextures.Add(Texture);
            (Desc.bWritable ? DefaultResources.NullRWTextures : DefaultResources.NullTextures)[Desc.NullTextureType] = Texture;
        }

        return true;
    };

    if (!CreateTextures(HeapTextures, false) || !CreateTextures(BufferTextures, true))
    {
        return false;
    }

    for (uint8 Component = 0; Component < static_cast<uint8>(EMSLTextureComponent::Count); ++Component)
    {
        for (uint8 Dimension = 0; Dimension < static_cast<uint8>(EMSLTextureDimension::Count); ++Dimension)
        {
            const EMSLTextureDimension TextureDimension = static_cast<EMSLTextureDimension>(Dimension);
            const EMSLTextureComponent TextureComponent = static_cast<EMSLTextureComponent>(Component);
            const uint8                NullTextureType  = MakeMSLNullTextureType(TextureDimension, TextureComponent);

            if (!DefaultResources.NullTextures[NullTextureType])
            {
                DefaultResources.NullTextures[NullTextureType] = DefaultResources.NullTextures[GetNullTextureFallback(TextureDimension, TextureComponent, false)];
            }

            if (!DefaultResources.NullRWTextures[NullTextureType])
            {
                DefaultResources.NullRWTextures[NullTextureType] = DefaultResources.NullRWTextures[GetNullTextureFallback(TextureDimension, TextureComponent, true)];
            }

            CHECK(DefaultResources.NullTextures[NullTextureType] != nil && DefaultResources.NullRWTextures[NullTextureType] != nil);
        }
    }

    id<MTLBuffer> Zeros = [[Device newBufferWithLength:16 options:MTLResourceStorageModeShared] autorelease];
    Memory::Memzero(Zeros.contents, 16);

    id<MTLCommandBuffer> CommandBuffer = [GetQueue(EMetalQueueType::Direct)->GetMTLCommandQueue() commandBuffer];
    CommandBuffer.label = @"MetalRHI.DefaultResources";

    id<MTLBlitCommandEncoder> BlitEncoder = [CommandBuffer blitCommandEncoder];
    [BlitEncoder fillBuffer:DefaultResources.NullBuffer range:NSMakeRange(0, FMetalDefaultResources::NullBufferSize) value:0];
    for (id<MTLTexture> Texture : DefaultResources.OwnedTextures)
    {
        if (Texture.buffer == nil && Texture.sampleCount == 1 && Texture.pixelFormat != MTLPixelFormatDepth32Float)
        {
            EncodeZeroFill(BlitEncoder, Zeros, Texture);
        }
    }
    [BlitEncoder endEncoding];

    for (id<MTLTexture> Texture : DefaultResources.OwnedTextures)
    {
        if (Texture.sampleCount > 1 || Texture.pixelFormat == MTLPixelFormatDepth32Float)
        {
            EncodeClearPasses(CommandBuffer, Texture);
        }
    }

    [CommandBuffer commit];
    [CommandBuffer waitUntilCompleted];

    METAL_INFO("Created %d null textures and a %u KB null buffer in a %llu KB heap",
        DefaultResources.OwnedTextures.Size(), FMetalDefaultResources::NullBufferSize / 1024, static_cast<uint64>(DefaultResources.Heap.size) / 1024ull);

    MTLSamplerDescriptor* SamplerDesc = [MTLSamplerDescriptor new];
    SamplerDesc.minFilter    = MTLSamplerMinMagFilterLinear;
    SamplerDesc.magFilter    = MTLSamplerMinMagFilterLinear;
    SamplerDesc.mipFilter    = MTLSamplerMipFilterLinear;
    SamplerDesc.sAddressMode = MTLSamplerAddressModeClampToEdge;
    SamplerDesc.tAddressMode = MTLSamplerAddressModeClampToEdge;
    SamplerDesc.rAddressMode = MTLSamplerAddressModeClampToEdge;

    SamplerDesc.supportArgumentBuffers = YES;

    DefaultResources.DefaultSampler = [Device newSamplerStateWithDescriptor:SamplerDesc];
    [SamplerDesc release];

    if (!DefaultResources.DefaultSampler)
    {
        METAL_ERROR("Failed to create default sampler");
        return false;
    }

    return true;
}

void FMetalDevice::QueryDeviceFeatureSupport()
{
    GMetalFeatures.ArgumentBuffersTier         = Device.argumentBuffersSupport;
    GMetalFeatures.bAppleGPU                   = [Device supportsFamily:MTLGPUFamilyApple7];
    GMetalFeatures.bMetal3                     = [Device supportsFamily:MTLGPUFamilyMetal3];
    GMetalFeatures.bUnifiedMemory              = Device.hasUnifiedMemory;
    GMetalFeatures.bDepth24Stencil8            = Device.depth24Stencil8PixelFormatSupported;
    GMetalFeatures.bStageBoundaryTimestamps    = [Device supportsCounterSampling:MTLCounterSamplingPointAtStageBoundary];
    GMetalFeatures.bDrawBoundaryTimestamps     = [Device supportsCounterSampling:MTLCounterSamplingPointAtDrawBoundary];
    GMetalFeatures.bDispatchBoundaryTimestamps = [Device supportsCounterSampling:MTLCounterSamplingPointAtDispatchBoundary];
    GMetalFeatures.bBlitBoundaryTimestamps     = [Device supportsCounterSampling:MTLCounterSamplingPointAtBlitBoundary];
    GMetalFeatures.bRayTracing                 = Device.supportsRaytracing;
    GMetalFeatures.bRayTracingFromRender       = Device.supportsRaytracingFromRender;
    GMetalFeatures.bHardwareRayTracing         = [Device supportsFamily:MTLGPUFamilyApple9];

#if METAL_SDK_HAS_MACOS_26
    if (@available(macOS 26.0, *))
    {
        GMetalSupportsDepthBoundsTest = true;
        GMetalSupportsSamplerLODBias  = true;
    }
#endif

    GMetalFeatures.MaxVertexAmplificationCount = 1;
    for (uint8 Count = 8; Count > 1; --Count)
    {
        if ([Device supportsVertexAmplificationCount:Count])
        {
            GMetalFeatures.MaxVertexAmplificationCount = Count;
            break;
        }
    }

    GMetalFeatures.SupportedSampleCounts = 0;
    for (uint32 SampleCount = 1; SampleCount <= RHI_MAX_SAMPLE_COUNT; SampleCount <<= 1)
    {
        if ([Device supportsTextureSampleCount:SampleCount])
        {
            GMetalFeatures.SupportedSampleCounts |= static_cast<uint8>(SampleCount);
        }
    }

    if (CVarForceMetal2Features.GetValue())
    {
#if METAL_ASSUME_APPLE_GPU
        METAL_WARNING("MetalRHI.ForceMetal2Features is ignored in the arm64 slice, which assumes a Metal 3 Apple GPU");
#else
        METAL_WARNING("MetalRHI.ForceMetal2Features is set, masking the device down to Metal 2 features");
        GMetalFeatures.ArgumentBuffersTier         = MTLArgumentBuffersTier1;
        GMetalFeatures.bMetal3                     = false;
        GMetalFeatures.MaxVertexAmplificationCount = 1;
#endif
    }

    GMetalHighestGPUFamily                    = Properties.HighestSupportedFamily;
    GMetalReadWriteTextureTier                = Device.readWriteTextureSupport;
    GMetalSupportsMeshShaders                 = MetalRHI::SupportsMetal3() && CVarEnableMeshShaders.GetValue();
    GMetalMaxBufferLength                     = static_cast<uint64>(Device.maxBufferLength);
    GMetalMaxThreadsPerThreadgroup            = static_cast<uint32>(Device.maxThreadsPerThreadgroup.width);
    GMetalMaxTextureArrayLayers               = 2048;
    GMetalMaxTexture3DSize                    = 2048;
    GMetalSupportsBCTextureCompression        = Device.supportsBCTextureCompression;
    GMetalSupportsProgrammableSamplePositions = Device.areProgrammableSamplePositionsSupported;
    GMetalSupportsStencilResolve              = [Device supportsFamily:MTLGPUFamilyApple5] || [Device supportsFamily:MTLGPUFamilyMac2];

    GMetalSupportsCounterSampling =
        GMetalFeatures.bStageBoundaryTimestamps ||
        GMetalFeatures.bDrawBoundaryTimestamps ||
        GMetalFeatures.bDispatchBoundaryTimestamps ||
        GMetalFeatures.bBlitBoundaryTimestamps;

    GMetalMaxTexture2DSize = 8192;

    if ([Device supportsFamily:MTLGPUFamilyApple3] || [Device supportsFamily:MTLGPUFamilyMac2])
    {
        GMetalMaxTexture2DSize = 16384;
    }
}

id<MTLDepthStencilState> FMetalDevice::GetDepthStencilState(const FRHIDepthStencilStateDesc& Desc)
{
    TScopedLock Lock(DepthStencilStatesCS);

    if (id<MTLDepthStencilState>* ExistingState = DepthStencilStates.Find(Desc))
    {
        return *ExistingState;
    }

    SCOPED_AUTORELEASE_POOL();

    MTLDepthStencilDescriptor* Descriptor = [[MTLDepthStencilDescriptor new] autorelease];
    Descriptor.depthWriteEnabled    = Desc.bDepthEnable && Desc.bDepthWriteEnable ? YES : NO;
    Descriptor.depthCompareFunction = Desc.bDepthEnable
        ? MetalRHI::ConvertCompareFunction(Desc.DepthFunc)
        : MTLCompareFunctionAlways;

    if (Desc.bStencilEnable)
    {
        const auto CreateStencilDescriptor = [&Desc](const FRHIDepthStencilStateDesc::FStencilState& Face)
        {
            MTLStencilDescriptor* StencilDescriptor = [[MTLStencilDescriptor new] autorelease];
            StencilDescriptor.stencilCompareFunction    = MetalRHI::ConvertCompareFunction(Face.StencilFunc);
            StencilDescriptor.stencilFailureOperation   = MetalRHI::ConvertStencilOp(Face.StencilFailOp);
            StencilDescriptor.depthFailureOperation     = MetalRHI::ConvertStencilOp(Face.StencilDepthFailOp);
            StencilDescriptor.depthStencilPassOperation = MetalRHI::ConvertStencilOp(Face.StencilDepthPassOp);
            StencilDescriptor.readMask                  = Desc.StencilReadMask;
            StencilDescriptor.writeMask                 = Desc.StencilWriteMask;
            return StencilDescriptor;
        };

        Descriptor.frontFaceStencil = CreateStencilDescriptor(Desc.FrontFace);
        Descriptor.backFaceStencil  = CreateStencilDescriptor(Desc.BackFace);
    }

    id<MTLDepthStencilState> State = [Device newDepthStencilStateWithDescriptor:Descriptor];

    if (!State)
    {
        METAL_ERROR("Failed to create a MTLDepthStencilState");
        return nil;
    }

    DepthStencilStates.Add(Desc, State);
    return State;
}

void FMetalDevice::PublishUploadValue(uint64 Value)
{
    uint64 Current = LatestUploadValue.Load();
    while (Value > Current && !LatestUploadValue.CompareExchange(Value, Current))
    {
        Current = LatestUploadValue.Load();
    }
}

bool FMetalDevice::QueryVideoMemoryInfo(EVideoMemoryType Type, FRHIVideoMemoryInfo& OutInfo) const
{
    if (!Device)
    {
        OutInfo = FRHIVideoMemoryInfo();
        return false;
    }

    OutInfo.MemoryType = Type;

    const uint64 Allocated = Device.currentAllocatedSize;
    const uint64 Shared    = Math::Min<uint64>(static_cast<uint64>(Math::Max<int64>(SharedBytes.Load(), 0)), Allocated);
    const uint64 Managed   = static_cast<uint64>(Math::Max<int64>(ManagedBytes.Load(), 0));

    if (Type == EVideoMemoryType::Local)
    {
        OutInfo.MemoryBudget = ResidencyManager ? ResidencyManager->GetBudget() : Device.recommendedMaxWorkingSetSize;
        OutInfo.MemoryUsage  = Device.hasUnifiedMemory ? Allocated : Allocated - Shared;
    }
    else if (!Device.hasUnifiedMemory)
    {
        OutInfo.MemoryBudget = [NSProcessInfo processInfo].physicalMemory / 2;
        OutInfo.MemoryUsage  = Shared + Managed;
    }
    else
    {
        OutInfo.MemoryBudget = 0;
        OutInfo.MemoryUsage  = 0;
    }

    return true;
}

void FMetalDevice::TrackCPUVisibleBytes(MTLStorageMode StorageMode, int64 Delta)
{
    if (StorageMode == MTLStorageModeShared)
    {
        SharedBytes.Add(Delta);
    }
    else if (StorageMode == MTLStorageModeManaged)
    {
        ManagedBytes.Add(Delta);
    }
}

void FMetalDevice::WaitForGPU()
{
    ForEachQueue([](FMetalQueue& Queue)
    {
        Queue.WaitForCompletion();
    });
}

void FMetalDevice::ProcessQueues()
{
    ForEachQueue([](FMetalQueue& Queue)
    {
        Queue.ProcessCommandQueue();
    });

    if (StagingBufferAllocator)
    {
        StagingBufferAllocator->CleanUp();
    }

    if (DynamicConstantsAllocator)
    {
        DynamicConstantsAllocator->CleanUp();
    }

    if (UploadHeapAllocator)
    {
        UploadHeapAllocator->CleanUp();
    }

    if (BufferAllocator)
    {
        BufferAllocator->CleanUp();
    }

    if (TextureAllocator)
    {
        TextureAllocator->CleanUp();
    }

#if METAL_ENABLE_STATS
    UpdateMemoryStats();
#endif
}

#if METAL_ENABLE_STATS
void FMetalDevice::UpdateMemoryStats()
{
    if (DynamicConstantsAllocator)
    {
        FMetalAllocatorUsage Usage;
        DynamicConstantsAllocator->UpdateMemoryStats(Usage);
        STAT_SET(STAT_Metal_DynamicConstantsAllocated, Usage.AllocatedBytes);
        STAT_SET(STAT_Metal_DynamicConstantsUsed,      Usage.UsedBytes);
        STAT_SET(STAT_Metal_DynamicConstantsPages,     Usage.NumBlocks);
    }

    if (StagingBufferAllocator)
    {
        FMetalAllocatorUsage Usage;
        StagingBufferAllocator->UpdateMemoryStats(Usage);
        STAT_SET(STAT_Metal_StagingAllocated, Usage.AllocatedBytes);
        STAT_SET(STAT_Metal_StagingUsed,      Usage.UsedBytes);
        STAT_SET(STAT_Metal_StagingPages,     Usage.NumBlocks);
    }
}

void FMetalDevice::LogMemoryStats()
{
    const float Interval = CVarLogMemoryStatsInterval.GetValue();

    if (Interval <= 0.0f || !Device)
    {
        return;
    }

    const uint64 CurrentTime = FPlatformTime::QueryPerformanceCounter();
    const uint64 Frequency   = FPlatformTime::QueryPerformanceFrequency();

    if (LastMemoryLogTime != 0 && static_cast<double>(CurrentTime - LastMemoryLogTime) / static_cast<double>(Frequency) < static_cast<double>(Interval))
    {
        return;
    }

    LastMemoryLogTime = CurrentTime;

    const auto ToMB = [](int64 Bytes) -> double
    {
        return static_cast<double>(Bytes) / (1024.0 * 1024.0);
    };

    METAL_INFO("[MemoryStats] Frame=%llu Device=%.1f/%.1f MB CommandBuffersAlive=%lld EncodersOpen=%lld "
        "DynamicConstants=%.1f MB (%lld pages) Staging=%.1f MB (%lld pages) UploadHeap=%.1f MB UploadPages=+%lld/-%lld "
        "BufferHeaps=%.1f MB (%lld) TextureHeaps=%.1f MB (%lld) StandaloneBuffers=%.1f MB (%lld) StandaloneTextures=%.1f MB (%lld) Bindless=%.1f MB "
        "Budget=%.1f MB Resident=%.1f MB Evicted=%.1f MB Pinned=%.1f MB Evictions=%lld DefragMoves=%lld (%.1f MB) DefragCancels=%lld",
        FrameCounter.Load(), ToMB(static_cast<int64>(Device.currentAllocatedSize)), ToMB(static_cast<int64>(Device.recommendedMaxWorkingSetSize)),
        STAT_GET(STAT_Metal_CommandBuffersAlive), STAT_GET(STAT_Metal_EncodersOpen),
        ToMB(STAT_GET(STAT_Metal_DynamicConstantsAllocated)), STAT_GET(STAT_Metal_DynamicConstantsPages), ToMB(STAT_GET(STAT_Metal_StagingAllocated)), STAT_GET(STAT_Metal_StagingPages),
        ToMB(STAT_GET(STAT_Metal_UploadHeapAllocated)), STAT_GET(STAT_Metal_UploadPagesCreated), STAT_GET(STAT_Metal_UploadPagesReleased),
        ToMB(STAT_GET(STAT_Metal_BufferHeapAllocated)), STAT_GET(STAT_Metal_BufferHeaps), ToMB(STAT_GET(STAT_Metal_TextureHeapAllocated)), STAT_GET(STAT_Metal_TextureHeaps),
        ToMB(STAT_GET(STAT_Metal_StandaloneBufferBytes)), STAT_GET(STAT_Metal_StandaloneBuffers), ToMB(STAT_GET(STAT_Metal_StandaloneTextureBytes)), STAT_GET(STAT_Metal_StandaloneTextures),
        ToMB(STAT_GET(STAT_Metal_BindlessTableBytes)),
        ToMB(static_cast<int64>(ResidencyManager->GetBudget())), ToMB(static_cast<int64>(ResidencyManager->GetResidentBytes())),
        ToMB(static_cast<int64>(ResidencyManager->GetEvictedBytes())), ToMB(static_cast<int64>(ResidencyManager->GetPinnedBytes())),
        STAT_GET(STAT_Metal_Evictions), STAT_GET(STAT_Metal_DefragMoves), ToMB(STAT_GET(STAT_Metal_DefragBytesMoved)), STAT_GET(STAT_Metal_DefragCancels));
}
#endif
