#pragma once
#include "MetalRHI/MetalCore.h"
#include "MetalRHI/MetalQueue.h"
#include "MetalRHI/MetalQuery.h"
#include "RHI/RHIDevice.h"

class FMetalDevice;

struct FMetalDeviceProperties
{
    String                 Name;
    uint64                 RegistryID;
    uint64                 RecommendedMaxWorkingSetSize;
    uint64                 MaxBufferLength;
    MTLSize                MaxThreadsPerThreadgroup;
    MTLGPUFamily           HighestSupportedFamily;
    MTLArgumentBuffersTier ArgumentBuffersTier;

    bool bHasUnifiedMemory : 1;
    bool bIsLowPower       : 1;
    bool bIsRemovable      : 1;
    bool bIsHeadless       : 1;
};

struct EMetalNullTextureType
{
    enum Type : uint8
    {
        Texture1D        = 0,
        Texture1DArray   = 1,
        Texture2D        = 2,
        Texture2DArray   = 3,
        TextureCube      = 4,
        TextureCubeArray = 5,
        Texture3D        = 6,
        Count            = 7,
    };
};

struct FMetalDefaultResources
{
    bool Initialize(FMetalDevice& Device);
    void Release();

    id<MTLTexture> GetNullTexture(EMetalNullTextureType::Type Type) const
    {
        CHECK(Type < EMetalNullTextureType::Count);
        return NullTextures[Type];
    }

    // Metal has no writable cube textures, so those entries are nil
    id<MTLTexture> GetNullRWTexture(EMetalNullTextureType::Type Type) const
    {
        CHECK(Type < EMetalNullTextureType::Count);
        CHECK(NullRWTextures[Type] != nil);
        return NullRWTextures[Type];
    }

    id<MTLTexture>      NullTextures[EMetalNullTextureType::Count];
    id<MTLTexture>      NullRWTextures[EMetalNullTextureType::Count];
    id<MTLBuffer>       NullBuffer;
    id<MTLSamplerState> DefaultSampler;
};

class METALRHI_API FMetalDevice
{
public:

    // Returns a retained device, the caller owns the reference
    static id<MTLDevice> SelectDevice();

public:
    FMetalDevice();
    ~FMetalDevice();

    bool Initialize();
    bool QueryDeviceFeatureSupport();
    bool InitializeDefaultResources();

    void BeginFrame();
    void EndFrame();
    void WaitForGPU();

    bool SupportsFamily(MTLGPUFamily Family) const;
    bool QueryVideoMemoryInfo(EVideoMemoryType Type, FRHIVideoMemoryInfo& OutInfo) const;

    // Metal has a single general-purpose queue, so the type is only for interface parity
    FMetalQueue*        GetQueue(EMetalQueueType Type = EMetalQueueType::Direct) const;
    id<MTLCommandQueue> GetMTLCommandQueue() const;

    FMetalTimestampQueries& GetTimestampQueries() { return TimestampQueries; }
    FMetalOcclusionQueries& GetOcclusionQueries() { return OcclusionQueries; }

    id<MTLDevice>                 GetMTLDevice()        const { return Device; }
    const FMetalDeviceProperties& GetProperties()       const { return Properties; }
    FMetalDefaultResources&       GetDefaultResources()       { return DefaultResources; }

private:
    static int32 ScoreDevice(id<MTLDevice> CandidateDevice);

    void ReadDeviceProperties();

    id<MTLDevice>          Device;
    FMetalQueue*           Queue;
    FMetalDeviceProperties Properties;
    FMetalDefaultResources DefaultResources;
    FMetalTimestampQueries TimestampQueries;
    FMetalOcclusionQueries OcclusionQueries;
    uint64                 FrameCounter;
};
