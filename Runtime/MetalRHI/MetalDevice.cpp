#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalCapabilities.h"
#include "MetalRHI/MetalDeviceDebug.h"
#include "MetalRHI/MetalQueue.h"
#include "Core/Math/Math.h"
#include "Core/Memory/Memory.h"
#include "Core/Misc/ConsoleManager.h"
#include <objc/message.h>

static TAutoConsoleVariable<String> CVarPreferredDeviceName(
    "MetalRHI.PreferredDeviceName",
    "Selects the Metal device whose name contains this text, ignoring the device scoring when it matches",
    "");

static MTLTextureType GetNullMTLTextureType(EMetalNullTextureType::Type Type)
{
    switch (Type)
    {
        case EMetalNullTextureType::Texture1D:        return MTLTextureType1D;
        case EMetalNullTextureType::Texture1DArray:   return MTLTextureType1DArray;
        case EMetalNullTextureType::Texture2D:        return MTLTextureType2D;
        case EMetalNullTextureType::Texture2DArray:   return MTLTextureType2DArray;
        case EMetalNullTextureType::TextureCube:      return MTLTextureTypeCube;
        case EMetalNullTextureType::TextureCubeArray: return MTLTextureTypeCubeArray;
        case EMetalNullTextureType::Texture3D:        return MTLTextureType3D;
        default:
        {
            CHECK(false);
            return MTLTextureType2D;
        }
    }
}

static void EncodeZeroFill(id<MTLBlitCommandEncoder> Blit, id<MTLBuffer> Staging, id<MTLTexture> Texture, EMetalNullTextureType::Type Type)
{
    if (!Blit || !Staging || !Texture)
    {
        return;
    }

    const bool bIsTexture1D = Type == EMetalNullTextureType::Texture1D || Type == EMetalNullTextureType::Texture1DArray;
    const bool bIsTexture3D = Type == EMetalNullTextureType::Texture3D;

    const NSUInteger BytesPerPixel = 4;
    const NSUInteger BytesPerRow   = bIsTexture1D ? 0 : BytesPerPixel;
    const NSUInteger BytesPerImage = bIsTexture3D ? BytesPerPixel : 0;
    const NSUInteger SliceCount    = (Type == EMetalNullTextureType::TextureCube || Type == EMetalNullTextureType::TextureCubeArray) ? 6 : 1;

    for (NSUInteger Slice = 0; Slice < SliceCount; ++Slice)
    {
        [Blit copyFromBuffer:Staging
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

static id<MTLTexture> CreateNullTexture(id<MTLDevice> Device, EMetalNullTextureType::Type Type, MTLPixelFormat Format, MTLTextureUsage Usage)
{
    MTLTextureDescriptor* Descriptor = [MTLTextureDescriptor new];
    Descriptor.textureType      = GetNullMTLTextureType(Type);
    Descriptor.pixelFormat      = Format;
    Descriptor.width            = 1;
    Descriptor.height           = 1;
    Descriptor.depth            = 1;
    Descriptor.mipmapLevelCount = 1;
    Descriptor.sampleCount      = 1;
    Descriptor.arrayLength      = 1;
    Descriptor.usage            = Usage;
    Descriptor.storageMode      = MTLStorageModePrivate;
    Descriptor.cpuCacheMode     = MTLCPUCacheModeDefaultCache;

    id<MTLTexture> Texture = [Device newTextureWithDescriptor:Descriptor];
    [Descriptor release];
    return Texture;
}

bool FMetalDefaultResources::Initialize(FMetalDevice& InDevice)
{
    Memory::Memzero(NullTextures, sizeof(NullTextures));
    Memory::Memzero(NullRWTextures, sizeof(NullRWTextures));

    NullBuffer     = nil;
    DefaultSampler = nil;

    id<MTLDevice> DeviceHandle = InDevice.GetMTLDevice();

    for (uint8 Type = 0; Type < EMetalNullTextureType::Count; ++Type)
    {
        const EMetalNullTextureType::Type TextureType = static_cast<EMetalNullTextureType::Type>(Type);
        NullTextures[Type] = CreateNullTexture(DeviceHandle, TextureType, MTLPixelFormatRGBA8Unorm, MTLTextureUsageShaderRead);
        if (!NullTextures[Type])
        {
            METAL_ERROR("Failed to create null SRV texture %u", Type);
            return false;
        }
    }

    const MTLPixelFormat RWFormat = (GMetalReadWriteTextureTier >= MTLReadWriteTextureTier2)
        ? MTLPixelFormatRGBA8Unorm
        : MTLPixelFormatR32Uint;

    for (uint8 Type = 0; Type < EMetalNullTextureType::Count; ++Type)
    {
        const EMetalNullTextureType::Type TextureType = static_cast<EMetalNullTextureType::Type>(Type);
        if (TextureType == EMetalNullTextureType::TextureCube || TextureType == EMetalNullTextureType::TextureCubeArray)
        {
            NullRWTextures[Type] = nil;
            continue;
        }

        NullRWTextures[Type] = CreateNullTexture(
            DeviceHandle,
            TextureType,
            RWFormat,
            MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite);

        if (!NullRWTextures[Type])
        {
            METAL_ERROR("Failed to create null UAV texture %u", Type);
            return false;
        }
    }

    NullBuffer = [DeviceHandle newBufferWithLength:16 options:MTLResourceStorageModeShared];
    if (!NullBuffer)
    {
        METAL_ERROR("Failed to create null buffer");
        return false;
    }

    Memory::Memzero(NullBuffer.contents, 16);

    {
        FMetalUploadBatch UploadBatch(&InDevice);
        if (!UploadBatch.IsValid())
        {
            METAL_ERROR("Failed to create a blit encoder for default resources");
            return false;
        }

        for (uint8 Type = 0; Type < EMetalNullTextureType::Count; ++Type)
        {
            const EMetalNullTextureType::Type TextureType = static_cast<EMetalNullTextureType::Type>(Type);
            EncodeZeroFill(UploadBatch.GetBlitEncoder(), NullBuffer, NullTextures[Type], TextureType);
            EncodeZeroFill(UploadBatch.GetBlitEncoder(), NullBuffer, NullRWTextures[Type], TextureType);
        }
    }

    MTLSamplerDescriptor* SamplerDesc = [MTLSamplerDescriptor new];
    SamplerDesc.minFilter     = MTLSamplerMinMagFilterLinear;
    SamplerDesc.magFilter     = MTLSamplerMinMagFilterLinear;
    SamplerDesc.mipFilter     = MTLSamplerMipFilterLinear;
    SamplerDesc.sAddressMode  = MTLSamplerAddressModeClampToEdge;
    SamplerDesc.tAddressMode  = MTLSamplerAddressModeClampToEdge;
    SamplerDesc.rAddressMode  = MTLSamplerAddressModeClampToEdge;

    DefaultSampler = [DeviceHandle newSamplerStateWithDescriptor:SamplerDesc];
    [SamplerDesc release];

    if (!DefaultSampler)
    {
        METAL_ERROR("Failed to create default sampler");
        return false;
    }

    return true;
}

void FMetalDefaultResources::Release()
{
    for (uint8 Type = 0; Type < EMetalNullTextureType::Count; ++Type)
    {
        [NullTextures[Type] release];
        NullTextures[Type] = nil;

        [NullRWTextures[Type] release];
        NullRWTextures[Type] = nil;
    }

    [NullBuffer release];
    NullBuffer = nil;

    [DefaultSampler release];
    DefaultSampler = nil;
}

FMetalDevice::FMetalDevice()
    : Device(nil)
    , Queue(nullptr)
    , ComputeQueue(nullptr)
    , CopyQueue(nullptr)
    , Properties{}
    , DefaultResources{}
    , TimestampQueries(this)
    , OcclusionQueries(this)
    , FrameCounter(0)
{
    Memory::Memzero(DefaultResources.NullTextures, sizeof(DefaultResources.NullTextures));
    Memory::Memzero(DefaultResources.NullRWTextures, sizeof(DefaultResources.NullRWTextures));

    DefaultResources.NullBuffer     = nil;
    DefaultResources.DefaultSampler = nil;
}

FMetalDevice::~FMetalDevice()
{
    WaitForGPU();
    DefaultResources.Release();
    SAFE_DELETE(CopyQueue);
    SAFE_DELETE(ComputeQueue);
    SAFE_DELETE(Queue);

    [Device release];
    Device = nil;
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

    NSArray<id<MTLDevice>>* AvailableDevices = MTLCopyAllDevices();

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
        MTLGPUFamilyMac1,
        MTLGPUFamilyCommon3,
        MTLGPUFamilyCommon2,
        MTLGPUFamilyCommon1,
        MTLGPUFamilyMetal3,
    };

    for (MTLGPUFamily Family : Families)
    {
        if ([Device supportsFamily:Family])
        {
            Properties.HighestSupportedFamily = Family;
            break;
        }
    }

    METAL_INFO("Selected Device=%s", *Properties.Name);
}

bool FMetalDevice::Initialize()
{
    Device = SelectDevice();
    if (!Device)
    {
        METAL_ERROR("Failed to select a Metal device");
        return false;
    }

    ReadDeviceProperties();

    Queue = new FMetalQueue(this, EMetalQueueType::Direct);
    if (!Queue->Initialize())
    {
        METAL_ERROR("Failed to initialize FMetalQueue");
        return false;
    }

    ComputeQueue = new FMetalQueue(this, EMetalQueueType::Compute);
    if (!ComputeQueue->Initialize())
    {
        METAL_ERROR("Failed to initialize the Metal compute queue");
        return false;
    }

    CopyQueue = new FMetalQueue(this, EMetalQueueType::Copy);
    if (!CopyQueue->Initialize())
    {
        METAL_ERROR("Failed to initialize the Metal copy queue");
        return false;
    }

    if (!QueryDeviceFeatureSupport())
    {
        METAL_ERROR("Failed to query Metal device feature support");
        return false;
    }

    if (!InitializeDefaultResources())
    {
        METAL_ERROR("Failed to initialize default Metal resources");
        return false;
    }

    GMetalSupportsTimestampQueries = GMetalSupportsCounterSampling && TimestampQueries.Initialize();
    if (!GMetalSupportsTimestampQueries)
    {
        TimestampQueries.Release();
        METAL_INFO("Timestamp queries are unavailable on this Metal device");
    }
    else if (![Device supportsCounterSampling:MTLCounterSamplingPointAtStageBoundary])
    {
        METAL_INFO("Timestamp samples are quantized to encoder boundaries");
    }

    if (!OcclusionQueries.Initialize())
    {
        METAL_ERROR("Failed to initialize occlusion queries");
        return false;
    }

    DumpMetalCapabilities();
    return true;
}

bool FMetalDevice::QueryDeviceFeatureSupport()
{
    GMetalHighestGPUFamily             = Properties.HighestSupportedFamily;
    GMetalArgumentBuffersTier          = Device.argumentBuffersSupport;
    GMetalReadWriteTextureTier         = Device.readWriteTextureSupport;
    GMetalSupportsRayTracing           = Device.supportsRaytracing;
    GMetalSupportsRayTracingFromRender = Device.supportsRaytracingFromRender;
    GMetalSupportsMeshShaders          = [Device supportsFamily:MTLGPUFamilyApple9];
    GMetalSupportsUnifiedMemory        = Device.hasUnifiedMemory;
    GMetalMaxBufferLength              = static_cast<uint64>(Device.maxBufferLength);
    GMetalMaxThreadsPerThreadgroup     = static_cast<uint32>(Device.maxThreadsPerThreadgroup.width);
    GMetalMaxVertexAmplificationCount  = 1;

    if ([Device respondsToSelector:@selector(maxVertexAmplificationCount)])
    {
        const SEL Selector = @selector(maxVertexAmplificationCount);
        const NSUInteger AmplificationCount = ((NSUInteger (*)(id, SEL))objc_msgSend)(Device, Selector);
        GMetalMaxVertexAmplificationCount = Math::Max(static_cast<uint32>(AmplificationCount), 1u);
    }

    GMetalMaxTextureArrayLayers        = 2048;
    GMetalSupportsBCTextureCompression = Device.supportsBCTextureCompression;

    GMetalSupportsCounterSampling =
        [Device supportsCounterSampling:MTLCounterSamplingPointAtStageBoundary] ||
        [Device supportsCounterSampling:MTLCounterSamplingPointAtDrawBoundary] ||
        [Device supportsCounterSampling:MTLCounterSamplingPointAtDispatchBoundary] ||
        [Device supportsCounterSampling:MTLCounterSamplingPointAtBlitBoundary] ||
        [Device supportsCounterSampling:MTLCounterSamplingPointAtTileDispatchBoundary];

    GMetalMaxTexture2DSize = 8192;
    if ([Device supportsFamily:MTLGPUFamilyApple3] || [Device supportsFamily:MTLGPUFamilyMac2])
    {
        GMetalMaxTexture2DSize = 16384;
    }

    METAL_INFO("bSupportRayTracing=%s, bSupportRayTracingFromRender=%s, bSupportMeshShaders=%s",
        GMetalSupportsRayTracing ? "true" : "false",
        GMetalSupportsRayTracingFromRender ? "true" : "false",
        GMetalSupportsMeshShaders ? "true" : "false");

    return true;
}

bool FMetalDevice::InitializeDefaultResources()
{
    return DefaultResources.Initialize(*this);
}

void FMetalDevice::BeginFrame()
{
    FrameCounter++;
    MetalBeginFrameCapture(Device);
    ProcessQueues();
}

void FMetalDevice::EndFrame()
{
    ProcessQueues();
    MetalEndFrameCapture();
}

void FMetalDevice::WaitForGPU()
{
    if (Queue)
    {
        Queue->WaitForCompletion();
    }

    if (ComputeQueue)
    {
        ComputeQueue->WaitForCompletion();
    }

    if (CopyQueue)
    {
        CopyQueue->WaitForCompletion();
    }
}

void FMetalDevice::ProcessQueues()
{
    if (Queue)
    {
        Queue->ProcessCommandQueue();
    }

    if (ComputeQueue)
    {
        ComputeQueue->ProcessCommandQueue();
    }

    if (CopyQueue)
    {
        CopyQueue->ProcessCommandQueue();
    }
}

bool FMetalDevice::SupportsFamily(MTLGPUFamily Family) const
{
    return Device && [Device supportsFamily:Family];
}

bool FMetalDevice::QueryVideoMemoryInfo(EVideoMemoryType Type, FRHIVideoMemoryInfo& OutInfo) const
{
    if (!Device)
    {
        OutInfo = FRHIVideoMemoryInfo();
        return false;
    }

    OutInfo.MemoryType   = Type;
    OutInfo.MemoryBudget = Device.recommendedMaxWorkingSetSize;
    OutInfo.MemoryUsage  = Device.currentAllocatedSize;

    if (Type == EVideoMemoryType::NonLocal && Device.hasUnifiedMemory)
    {
        OutInfo.MemoryBudget = 0;
        OutInfo.MemoryUsage  = 0;
    }

    return true;
}

FMetalQueue* FMetalDevice::GetQueue(EMetalQueueType Type) const
{
    switch (Type)
    {
        case EMetalQueueType::Compute:
            return ComputeQueue;

        case EMetalQueueType::Copy:
            return CopyQueue;

        default:
            return Queue;
    }
}

id<MTLCommandQueue> FMetalDevice::GetMTLCommandQueue() const
{
    return Queue ? Queue->GetMTLCommandQueue() : nil;
}
