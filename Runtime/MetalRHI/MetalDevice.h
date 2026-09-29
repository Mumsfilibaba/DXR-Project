#pragma once
#include "Core/Containers/Map.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Threading/Atomic/AtomicBool.h"
#include "MetalRHI/MetalCore.h"
#include "MetalRHI/MetalQueue.h"
#include "MetalRHI/MetalQuery.h"
#include "RHI/RHIDevice.h"
#include "RHI/RHIPipelineState.h"

class FMetalDevice;
class FMetalResidencySet;
class FMetalUploadHeapAllocator;
class FMetalLinearAllocator;
class FMetalBufferAllocator;
class FMetalTextureAllocator;
class FMetalBindlessDescriptorManager;

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

struct FMetalDefaultResources
{
    static constexpr uint32 NullBufferSize = 64 * 1024;

    id<MTLTexture> GetNullTexture(uint8 NullTextureType) const
    {
        CHECK(NullTextureType < MSL_NUM_NULL_TEXTURE_TYPES);
        return NullTextures[NullTextureType];
    }

    id<MTLTexture> GetNullRWTexture(uint8 NullTextureType) const
    {
        CHECK(NullTextureType < MSL_NUM_NULL_TEXTURE_TYPES);
        return NullRWTextures[NullTextureType];
    }

    id<MTLHeap>            Heap;
    TArray<id<MTLTexture>> OwnedTextures;
    id<MTLTexture>         NullTextures[MSL_NUM_NULL_TEXTURE_TYPES];
    id<MTLTexture>         NullRWTextures[MSL_NUM_NULL_TEXTURE_TYPES];
    id<MTLBuffer>          NullBuffer;
    id<MTLSamplerState>    DefaultSampler;
};

class METALRHI_API FMetalDevice
{
public:
    FMetalDevice();
    ~FMetalDevice();

    bool Initialize();
    void BeginFrame();
    void EndFrame();

    void WaitForGPU();
    void ProcessQueues();

    void PublishUploadValue(uint64 Value);

    bool QueryVideoMemoryInfo(EVideoMemoryType Type, FRHIVideoMemoryInfo& OutInfo) const;

    id<MTLDepthStencilState> GetDepthStencilState(const FRHIDepthStencilStateDesc& Desc);

    FMetalResidencySet&              GetResidencySet()              const { return *ResidencySet; }
    FMetalBindlessDescriptorManager* GetBindlessDescriptorManager() const { return BindlessDescriptorManager; }
    FMetalLinearAllocator*           GetStagingBufferAllocator()    const { return StagingBufferAllocator; }
    FMetalLinearAllocator*           GetDynamicConstantsAllocator() const { return DynamicConstantsAllocator; }
    FMetalBufferAllocator*           GetBufferAllocator()           const { return BufferAllocator; }
    FMetalTextureAllocator*          GetTextureAllocator()          const { return TextureAllocator; }
    FMetalUploadHeapAllocator*       GetUploadHeapAllocator()       const { return UploadHeapAllocator; }
    FMetalTimestampQueries&          GetTimestampQueries()                { return TimestampQueries; }
    FMetalOcclusionQueries&          GetOcclusionQueries()                { return OcclusionQueries; }
    const FMetalDefaultResources&    GetDefaultResources()          const { return DefaultResources; }
    const FMetalDeviceProperties&    GetProperties()                const { return Properties; }

    template<typename FunctionType>
    FORCEINLINE void ForEachQueue(FunctionType&& Function) const
    {
        for (FMetalQueue* Queue : Queues)
        {
            if (Queue)
            {
                Function(*Queue);
            }
        }
    }

    FMetalQueue* GetQueue(EMetalQueueType QueueType) const
    {
        return Queues[static_cast<uint32>(QueueType)];
    }

    uint64 GetLatestUploadValue() const
    {
        return LatestUploadValue.Load();
    }

    FORCEINLINE id<MTLDevice> GetMTLDevice() const
    {
        return Device;
    }

private:
    static int32 ScoreDevice(id<MTLDevice> CandidateDevice);

    id<MTLDevice> SelectDevice();
    void ReadDeviceProperties();

    bool CreateDevice();
    bool CreateCommandQueues();
    bool CreateDefaultResources();
    void QueryDeviceFeatureSupport();

#if METAL_ENABLE_STATS
    void UpdateMemoryStats();
    void LogMemoryStats();
#endif

    FMetalResidencySet*                                                     ResidencySet;
    FMetalBindlessDescriptorManager*                                        BindlessDescriptorManager;
    FMetalLinearAllocator*                                                  StagingBufferAllocator;
    FMetalLinearAllocator*                                                  DynamicConstantsAllocator;
    FMetalBufferAllocator*                                                  BufferAllocator;
    FMetalTextureAllocator*                                                 TextureAllocator;
    FMetalUploadHeapAllocator*                                              UploadHeapAllocator;
    TStaticArray<FMetalQueue*, static_cast<uint32>(EMetalQueueType::Count)> Queues;
    FMetalTimestampQueries                                                  TimestampQueries;
    FMetalOcclusionQueries                                                  OcclusionQueries;
    FMetalDefaultResources                                                  DefaultResources;
    TMap<FRHIDepthStencilStateDesc, id<MTLDepthStencilState>>               DepthStencilStates;
    FCriticalSection                                                        DepthStencilStatesCS;
    TAtomicInt<uint64>                                                      LatestUploadValue;
    uint64                                                                  FrameCounter;
    uint64                                                                  LastMemoryLogTime;
    FMetalDeviceProperties                                                  Properties;
    id<MTLDevice>                                                           Device;
    id<NSObject>                                                            DeviceObserver;
    AtomicBool                                                              bDeviceRemoved;
};
