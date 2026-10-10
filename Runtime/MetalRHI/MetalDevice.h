#pragma once
#include "Core/Containers/Map.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Threading/Atomic/AtomicBool.h"
#include "MetalRHI/MetalAllocators.h"
#include "MetalRHI/MetalCore.h"
#include "MetalRHI/MetalQueue.h"
#include "MetalRHI/MetalQuery.h"
#include "RHI/RHIDevice.h"
#include "RHI/RHIPipelineState.h"

class FMetalDevice;
class FMetalCommandContext;
class FMetalResidencySet;
class FMetalResidencyManager;
class FMetalShaderLibraryCache;
class FMetalPipelineCache;
class FMetalBinaryArchive;
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
    void TrackCPUVisibleBytes(MTLStorageMode StorageMode, int64 Delta);

    id<MTLDepthStencilState> GetDepthStencilState(const FRHIDepthStencilStateDesc& Desc);

    uint32 RecordDefragMoves(FMetalCommandContext& Context);
    void   SetDefragWaitValues(uint64 DirectValue);

    void FinalizeDefragMoves();
    void CancelDefragMove(FMetalResourceStorage& Storage);

    void TrimAllocatorCaches();

    FORCEINLINE bool HasPendingDefragMoves() const { return bHasPendingDefragMoves.Load(); }

    FORCEINLINE id<MTLDevice>                    GetMTLDevice()                 const { return Device; }
    FORCEINLINE uint64                           GetFrameCounter()              const { return FrameCounter.Load(); }
    FORCEINLINE uint64                           GetLatestUploadValue()         const { return LatestUploadValue.Load(); }
    FORCEINLINE FMetalResidencySet&              GetResidencySet()              const { return *ResidencySet; }
    FORCEINLINE FMetalResidencyManager&          GetResidencyManager()          const { return *ResidencyManager; }
    FORCEINLINE FMetalShaderLibraryCache&        GetShaderLibraryCache()        const { return *ShaderLibraryCache; }
    FORCEINLINE FMetalPipelineCache&             GetPipelineCache()             const { return *PipelineCache; }
    FORCEINLINE FMetalBinaryArchive&             GetBinaryArchive()             const { return *BinaryArchive; }
    FORCEINLINE FMetalBindlessDescriptorManager* GetBindlessDescriptorManager() const { return BindlessDescriptorManager; }
    FORCEINLINE FMetalLinearAllocator*           GetStagingBufferAllocator()    const { return StagingBufferAllocator; }
    FORCEINLINE FMetalLinearAllocator*           GetDynamicConstantsAllocator() const { return DynamicConstantsAllocator; }
    FORCEINLINE FMetalBufferAllocator*           GetBufferAllocator()           const { return BufferAllocator; }
    FORCEINLINE FMetalTextureAllocator*          GetTextureAllocator()          const { return TextureAllocator; }
    FORCEINLINE FMetalUploadHeapAllocator*       GetUploadHeapAllocator()       const { return UploadHeapAllocator; }
    FORCEINLINE FMetalTimestampQueries&          GetTimestampQueries()                { return TimestampQueries; }
    FORCEINLINE FMetalOcclusionQueries&          GetOcclusionQueries()                { return OcclusionQueries; }
    FORCEINLINE FMetalStatisticQueries&          GetStatisticQueries()                { return StatisticQueries; }
    FORCEINLINE FMetalStageUtilizationQueries&   GetStageUtilizationQueries()         { return StageUtilizationQueries; }
    FORCEINLINE const FMetalDefaultResources&    GetDefaultResources()          const { return DefaultResources; }
    FORCEINLINE const FMetalDeviceProperties&    GetProperties()                const { return Properties; }

    FORCEINLINE FMetalQueue* GetQueue(EMetalQueueType QueueType) const { return Queues[static_cast<uint32>(QueueType)]; }

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

private:
    static int32 ScoreDevice(id<MTLDevice> CandidateDevice);

    id<MTLDevice> SelectDevice();
    void ReadDeviceProperties();

    bool CreateDevice();
    bool CreateCommandQueues();
    bool CreateDefaultResources();
    bool CreatePipelineCaches();
    void QueryDeviceFeatureSupport();

#if METAL_ENABLE_STATS
    void UpdateMemoryStats();
    void LogMemoryStats();
#endif

    FMetalResidencySet*                                                     ResidencySet;
    FMetalResidencyManager*                                                 ResidencyManager;
    FMetalShaderLibraryCache*                                               ShaderLibraryCache;
    FMetalPipelineCache*                                                    PipelineCache;
    FMetalBinaryArchive*                                                    BinaryArchive;
    FMetalBindlessDescriptorManager*                                        BindlessDescriptorManager;
    FMetalLinearAllocator*                                                  StagingBufferAllocator;
    FMetalLinearAllocator*                                                  DynamicConstantsAllocator;
    FMetalBufferAllocator*                                                  BufferAllocator;
    FMetalTextureAllocator*                                                 TextureAllocator;
    FMetalUploadHeapAllocator*                                              UploadHeapAllocator;
    TStaticArray<FMetalQueue*, static_cast<uint32>(EMetalQueueType::Count)> Queues;
    FMetalTimestampQueries                                                  TimestampQueries;
    FMetalOcclusionQueries                                                  OcclusionQueries;
    FMetalStatisticQueries                                                  StatisticQueries;
    FMetalStageUtilizationQueries                                           StageUtilizationQueries;
    FMetalDefaultResources                                                  DefaultResources;
    TMap<FRHIDepthStencilStateDesc, id<MTLDepthStencilState>>               DepthStencilStates;
    FCriticalSection                                                        DepthStencilStatesCS;
    TAtomicInt<uint64>                                                      LatestUploadValue;
    TArray<FMetalDefragMove>                                                PendingDefragMoves;
    TStaticArray<uint64, static_cast<uint32>(EMetalQueueType::Count)>       DefragWaitValues;
    FCriticalSection                                                        DefragCS;
    AtomicBool                                                              bHasPendingDefragMoves;
    TAtomicInt<uint64>                                                      FrameCounter;
    TAtomicInt<int64>                                                       SharedBytes;
    TAtomicInt<int64>                                                       ManagedBytes;
    uint64                                                                  LastMemoryLogTime;
    FMetalDeviceProperties                                                  Properties;
    id<MTLDevice>                                                           Device;
    id<NSObject>                                                            DeviceObserver;
    AtomicBool                                                              bDeviceRemoved;
};
