#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Queue.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Platform/CriticalSection.h"
#include "D3D12RHI/D3D12DeviceChild.h"
#include "D3D12RHI/D3D12RefCounted.h"

class FD3D12DescriptorHeap;
class FD3D12OnlineDescriptorHeap;

typedef TSharedRef<FD3D12DescriptorHeap> FD3D12DescriptorHeapRef;

class FD3D12DescriptorHeap : public FD3D12DeviceChild, public FD3D12RefCounted
{
public:
    FD3D12DescriptorHeap(FD3D12Device* InDevice, ID3D12DescriptorHeap* InHeap, D3D12_DESCRIPTOR_HEAP_TYPE InType, D3D12_DESCRIPTOR_HEAP_FLAGS InFlags, uint32 InNumDescriptors);
    FD3D12DescriptorHeap(FD3D12DescriptorHeap* InHeap, uint32 InHandleOffset, uint32 InNumDescriptors);

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(int32 Index) const { return FD3D12_CPU_DESCRIPTOR_HANDLE(StartHandleCPU, Index, HandleIncrementSize); }
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(int32 Index) const { return FD3D12_GPU_DESCRIPTOR_HANDLE(StartHandleGPU, Index, HandleIncrementSize); }
    uint32 GetNumDescriptors()      const { return NumDescriptors; }
    uint32 GetHandleIncrementSize() const { return HandleIncrementSize; }

    ID3D12DescriptorHeap* GetD3D12Heap() const
    {
        return Heap.Get();
    }

    D3D12_DESCRIPTOR_HEAP_TYPE GetType() const
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
        , HeapIndex{0}
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
    FD3D12OfflineDescriptor Allocate();
    void Free(FD3D12OfflineDescriptor& Descriptor);

    uint32 GetNumDescriptors() const
    {
        return NumDescriptors;
    }

    uint32 GetDescriptorSize() const
    {
        return DescriptorSize;
    }

    D3D12_DESCRIPTOR_HEAP_TYPE GetType() const
    {
        return Type;
    }

private:
    bool AllocateHeap();
    
    D3D12_DESCRIPTOR_HEAP_TYPE const Type;
    uint32                           DescriptorSize;
    uint32                           NumDescriptors;

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

    TArray<FOfflineHeap> Heaps;
    FCriticalSection     HeapsCS;
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

    bool Initialize(uint32 InDescriptorCount, uint32 BlockSize);
    FD3D12OnlineDescriptorBlock* AllocateBlock();
    void RecycleBlock(FD3D12OnlineDescriptorBlock* InBlock);
    void FreeBlockDeferred(FD3D12OnlineDescriptorBlock* InBlock);

    FD3D12DescriptorHeap* GetHeap() const
    { 
        return Heap.Get();
    }

    uint32 GetBlockSize() const
    {
        return BlockSize;
    }

    uint32 GetNumDescriptors() const
    {
        return DescriptorCount;
    }

private:
    D3D12_DESCRIPTOR_HEAP_TYPE const     Type;
    uint32                               DescriptorCount;
    uint32                               BlockSize;
    FD3D12DescriptorHeapRef              Heap;
    TQueue<FD3D12OnlineDescriptorBlock*> AvailableBlockQueue;
    TArray<FD3D12OnlineDescriptorBlock*> BlockQueue;
    FCriticalSection                     BlockQueueCS;
};
