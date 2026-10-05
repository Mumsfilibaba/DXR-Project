#pragma once
#include "Core/Containers/Array.h"
#include "Core/Templates/Utility/EnumOperators.h"
#include "Core/Templates/Utility/NonCopyable.h"
#include "Core/Threading/Atomic/AtomicBool.h"
#include "MetalRHI/MetalDeviceChild.h"
#include "MetalRHI/MetalResidencyManager.h"
#include "MetalRHI/MetalStageEncoder.h"

class FMetalQueue;
struct FMetalCommands;

enum class EMetalEncoderType : uint8
{
    None = 0,
    Render,
    Compute,
    Blit,
    AccelerationStructure,
    Count,
};

enum class EMetalClearAspects : uint8
{
    None    = 0,
    Color   = FLAG(0),
    Depth   = FLAG(1),
    Stencil = FLAG(2),
};
ENUM_CLASS_OPERATORS(EMetalClearAspects);

struct FMetalPendingClear
{
    id<MTLTexture>     Texture     = nil;
    NSUInteger         Level       = 0;
    NSUInteger         Slice       = 0;
    NSUInteger         DepthPlane  = 0;
    id<MTLTexture>     RootTexture = nil;
    NSUInteger         RootLevel   = 0;
    NSUInteger         RootSlice   = 0;
    MTLClearColor      Color       = {};
    double             Depth       = 0.0;
    uint32             Stencil     = 0;
    EMetalClearAspects Aspects     = EMetalClearAspects::None;
};

struct FMetalEncoderFence
{
    void Wait(id<MTLCommandEncoder> Encoder, EMetalEncoderType Type, const FMetalCommands& Commands);
    void Signal(id<MTLCommandEncoder> Encoder, EMetalEncoderType Type, FMetalCommands& Commands);

    id<MTLFence> Fence = nil;
    AtomicBool   bUpdateCommitted;
};

class FMetalEncoderManager : public FMetalDeviceChild, public FNonCopyAndNonMovable
{
public:
    explicit FMetalEncoderManager(FMetalQueue& InQueue);
    ~FMetalEncoderManager();

    void            BeginCommandBuffer(FMetalCommands* InCommands);
    FMetalCommands* EndCommandBuffer();

    id<MTLRenderCommandEncoder>                BeginRenderEncoder(MTLRenderPassDescriptor* Descriptor, const CHAR* Label);
    id<MTLRenderCommandEncoder>                AdoptRenderEncoder(id<MTLRenderCommandEncoder> SubEncoder, const CHAR* Label);
    id<MTLComputeCommandEncoder>               RequireComputeEncoder();
    id<MTLBlitCommandEncoder>                  RequireBlitEncoder();
    id<MTLAccelerationStructureCommandEncoder> RequireAccelerationStructureEncoder();
    id<MTLParallelRenderCommandEncoder>        BeginParallelRenderEncoder(MTLRenderPassDescriptor* Descriptor, const CHAR* Label);
    void                                       EndParallelRenderEncoder(id<MTLParallelRenderCommandEncoder> ParallelEncoder);

    void EncodeLoadStorePass(MTLRenderPassDescriptor* Descriptor, const CHAR* Label);
    void EndEncoder();
    void MemoryBarrier();

    void AddPendingClear(MTLRenderPassAttachmentDescriptor* Attachment, const FMetalPendingClear& Clear);
    void FlushPendingClears(id<MTLTexture> Texture);

    bool HasPendingClears() const { return !PendingClears.IsEmpty(); }

    void ScheduleTimestamp(uint32 SampleIndex);
#if !METAL_ASSUME_APPLE_GPU
    void SampleCounters(uint32 SampleIndex);
#endif

    EMetalEncoderType GetEncoderType() const { return EncoderType; }

    id<MTLRenderCommandEncoder> GetRenderEncoder() const
    {
        return EncoderType == EMetalEncoderType::Render
            ? static_cast<id<MTLRenderCommandEncoder>>(Encoder)
            : nil;
    }

    id<MTLComputeCommandEncoder> GetComputeEncoder() const
    {
        return EncoderType == EMetalEncoderType::Compute
            ? static_cast<id<MTLComputeCommandEncoder>>(Encoder)
            : nil;
    }

    uint64 GetEncoderSerial() const { return EncoderSerial; }

    bool HasCommands() const { return Commands != nullptr; }

    FMetalCommands&            GetCommands() const { CHECK(Commands != nullptr); return *Commands; }
    FMetalEncoderBindingCache& GetBindingCache()   { return BindingCache; }

    FORCEINLINE void UpdateResidency(FMetalResidencyEntry* Entry)
    {
        ResidencyList.Insert(Entry);
    }

    template<typename CommandEncoderType>
    void RefreshBindlessResidency(CommandEncoderType InEncoder);

private:
    template<EMetalEncoderType Type, typename OpenFunctionType>
    id<MTLCommandEncoder> OpenEncoder(OpenFunctionType&& Open, const CHAR* Label);
    void PrepareToOpen();
    void WaitForPublishedUploads();
    id<MTLRenderCommandEncoder> OpenRenderEncoder(MTLRenderPassDescriptor* Descriptor, const CHAR* Label);
    void ResolvePendingClears(MTLRenderPassDescriptor* Descriptor);
    void EncodePendingClear(const FMetalPendingClear& Clear);

    template<typename CommandEncoderType>
    void PrepareBindless(CommandEncoderType InEncoder);
    void EncodeTimestampPass(uint32 StartIndex, uint32 EndIndex);
    uint32 TakeScheduledTimestamp();

    FMetalQueue&               Queue;
    FMetalCommands*            Commands;
    id<MTLCommandEncoder>      Encoder;
    EMetalEncoderType          EncoderType;
    FMetalEncoderFence&        Fence;
    FMetalEncoderBindingCache  BindingCache;
    TArray<uint32>             ScheduledTimestamps;
    TArray<FMetalPendingClear> PendingClears;
    FMetalResidencyList        ResidencyList;
    uint64                     WaitedUploadValue;
    uint64                     DeclaredResidencyGeneration;
    uint64                     EncoderSerial;
    bool                       bAdoptedEncoder;
};
