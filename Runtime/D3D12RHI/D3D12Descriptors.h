#pragma once
#include "Core/RefCountedBase.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Queue.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Platform/CriticalSection.h"
#include "D3D12RHI/D3D12DeviceChild.h"
#include "RHI/RHITypes.h"

class FD3D12DescriptorHeap;
class FD3D12OnlineDescriptorHeap;
class FD3D12BindlessDescriptorHeap;

typedef TSharedRef<FD3D12DescriptorHeap> FD3D12DescriptorHeapRef;

class FD3D12DescriptorHeap : public FD3D12DeviceChild, public FRefCountedBase
{
public:
    FD3D12DescriptorHeap(FD3D12Device* InDevice, ID3D12DescriptorHeap* InHeap, D3D12_DESCRIPTOR_HEAP_TYPE InType, D3D12_DESCRIPTOR_HEAP_FLAGS InFlags, uint32 InNumDescriptors);
    FD3D12DescriptorHeap(FD3D12DescriptorHeap* InHeap, uint32 InHandleOffset, uint32 InNumDescriptors);
    ~FD3D12DescriptorHeap() = default;
    
    NODISCARD FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(int32 Index) const
    {
        return FD3D12_CPU_DESCRIPTOR_HANDLE(StartHandleCPU, Index, HandleIncrementSize);
    }

    NODISCARD FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(int32 Index) const
    {
        return FD3D12_GPU_DESCRIPTOR_HANDLE(StartHandleGPU, Index, HandleIncrementSize);
    }

    NODISCARD FORCEINLINE uint32 GetNumDescriptors() const
    {
        return NumDescriptors;
    }

    NODISCARD FORCEINLINE uint32 GetHandleIncrementSize() const
    {
        return HandleIncrementSize;
    }

    NODISCARD FORCEINLINE ID3D12DescriptorHeap* GetD3D12Heap() const
    {
        return Heap.Get();
    }

    NODISCARD FORCEINLINE D3D12_DESCRIPTOR_HEAP_TYPE GetType() const
    {
        return Type;
    }

private:
    TComPtr<ID3D12DescriptorHeap> Heap;
    D3D12_CPU_DESCRIPTOR_HANDLE   StartHandleCPU;
    D3D12_GPU_DESCRIPTOR_HANDLE   StartHandleGPU;
    D3D12_DESCRIPTOR_HEAP_TYPE    Type;
    D3D12_DESCRIPTOR_HEAP_FLAGS   Flags;
    uint32                        NumDescriptors;
    uint32                        HandleIncrementSize;
};

struct FD3D12DescriptorRange
{
    FD3D12DescriptorRange() = default;

    FD3D12DescriptorRange(D3D12_CPU_DESCRIPTOR_HANDLE InStart, D3D12_CPU_DESCRIPTOR_HANDLE InEnd)
        : Start(InStart)
        , End(InEnd)
    {
    }

    bool IsValid() const
    {
        return Start.ptr < End.ptr;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE Start = { 0 };
    D3D12_CPU_DESCRIPTOR_HANDLE End   = { 0 };
};

struct FD3D12OfflineDescriptor
{
    FD3D12OfflineDescriptor()
        : Handle{0}
        , HeapIndex(0)
    {
    }

    FD3D12OfflineDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE InHandle, int32 InHeapIndex)
        : Handle(InHandle)
        , HeapIndex(InHeapIndex)
    {
    }

    operator bool() const
    {
        return Handle.ptr != 0;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE Handle;
    int32                       HeapIndex;
};

class FD3D12OfflineDescriptorHeap : public FD3D12DeviceChild
{
public:
    FD3D12OfflineDescriptorHeap(FD3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType);
    ~FD3D12OfflineDescriptorHeap() = default;

    bool Initialize();

    NODISCARD FD3D12OfflineDescriptor Allocate();
    void Free(FD3D12OfflineDescriptor& Descriptor);

    NODISCARD FORCEINLINE uint32 GetNumTotalDescriptors() const
    {
        return NumTotalDescriptors;
    }

    NODISCARD FORCEINLINE uint32 GetDescriptorSize() const
    {
        return DescriptorSize;
    }

    NODISCARD FORCEINLINE D3D12_DESCRIPTOR_HEAP_TYPE GetType() const
    {
        return Type;
    }

private:
    bool AllocateHeap();
    
    struct FOfflineHeap
    {
        FOfflineHeap(const FD3D12DescriptorHeapRef& InHeap, uint32 InNumDescriptors)
            : FreeList()
            , Heap(InHeap)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE Start = InHeap->GetCPUHandle(0);
            D3D12_CPU_DESCRIPTOR_HANDLE End   = FD3D12_CPU_DESCRIPTOR_HANDLE(Start, InNumDescriptors, InHeap->GetHandleIncrementSize());
            FreeList.Emplace(Start, End);
        }

        FD3D12DescriptorHeapRef       Heap;
        TArray<FD3D12DescriptorRange> FreeList;
    };

    D3D12_DESCRIPTOR_HEAP_TYPE const Type;
    uint32                           DescriptorSize;
    uint32                           NumTotalDescriptors;
    TArray<FOfflineHeap>             Heaps;
    FCriticalSection                 HeapsCS;
};

struct FD3D12OnlineDescriptorBlock
{
    FD3D12OnlineDescriptorBlock()
        : HandleOffset(0)
        , NumDescriptors(0)
    {
    }

    FD3D12OnlineDescriptorBlock(uint32 InHandleOffset, uint32 InNumDescriptors)
        : HandleOffset(InHandleOffset)
        , NumDescriptors(InNumDescriptors)
    {
    }

    uint32 HandleOffset;
    uint32 NumDescriptors;
};

class FD3D12OnlineDescriptorHeap : public FD3D12DeviceChild
{
public:
    FD3D12OnlineDescriptorHeap(FD3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType);
    ~FD3D12OnlineDescriptorHeap();

    bool Initialize(uint32 InDescriptorCount, uint32 InBlockSize, uint32 InBindlessReservedCount = 0);

    NODISCARD FD3D12OnlineDescriptorBlock* AllocateBlock();
    void RecycleBlock(FD3D12OnlineDescriptorBlock* InBlock);

    NODISCARD FORCEINLINE FD3D12DescriptorHeap* GetHeap() const
    { 
        return Heap.Get();
    }

    NODISCARD FORCEINLINE uint32 GetBlockSize() const
    {
        return BlockSize;
    }

    NODISCARD FORCEINLINE uint32 GetNumDescriptors() const
    {
        return DescriptorCount;
    }

    NODISCARD FORCEINLINE uint32 GetBindlessReservedCount() const
    {
        return BindlessReservedCount;
    }

    NODISCARD FORCEINLINE uint32 GetNumBlocks() const
    {
        return static_cast<uint32>(BlockQueue.Size());
    }

    NODISCARD bool HasAvailableBlock() const
    {
        TScopedLock Lock(BlockQueueCS);
        return !AvailableBlockQueue.IsEmpty();
    }

private:
    const D3D12_DESCRIPTOR_HEAP_TYPE     Type;
    uint32                               DescriptorCount;
    uint32                               BlockSize;
    uint32                               BindlessReservedCount;
    FD3D12DescriptorHeapRef              Heap;
    TQueue<FD3D12OnlineDescriptorBlock*> AvailableBlockQueue;
    TArray<FD3D12OnlineDescriptorBlock*> BlockQueue;
    mutable FCriticalSection             BlockQueueCS;
};

struct FD3D12PendingBindlessWrite
{
    uint32                      DestSlot   = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE SrcHandle  = { 0 };
};

class FD3D12BindlessDescriptorHeap : public FD3D12DeviceChild
{
public:
    FD3D12BindlessDescriptorHeap(FD3D12OnlineDescriptorHeap& InGlobalHeap, uint32 InCapacity);
    ~FD3D12BindlessDescriptorHeap();

    NODISCARD FRHIDescriptorHandle Allocate(EDescriptorType InType);

    void Free(FRHIDescriptorHandle Handle);
    void RecycleSlot(FRHIDescriptorHandle Handle);
    void EnqueueWrite(FRHIDescriptorHandle Handle, D3D12_CPU_DESCRIPTOR_HANDLE OfflineHandle);
    void WriteSlotImmediate(FRHIDescriptorHandle Handle, D3D12_CPU_DESCRIPTOR_HANDLE OfflineHandle);
    void Flush();

    NODISCARD FORCEINLINE FD3D12DescriptorHeap* GetAliasedHeap() const
    {
        return AliasedHeap.Get();
    }

    NODISCARD FORCEINLINE D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const
    {
        return HeapType;
    }

    NODISCARD FORCEINLINE uint32 GetCapacity() const
    {
        return Capacity;
    }

private:
    FD3D12DescriptorHeapRef             AliasedHeap;
    D3D12_DESCRIPTOR_HEAP_TYPE          HeapType;
    uint32                              Capacity;
    uint32                              NextFreshSlot;
    TArray<uint32>                      FreeStack;
    TArray<D3D12_CPU_DESCRIPTOR_HANDLE> SlotSources;
    FCriticalSection                    AllocCS;
    TArray<FD3D12PendingBindlessWrite>  PendingWrites;
    FCriticalSection                    PendingWritesCS;
};
