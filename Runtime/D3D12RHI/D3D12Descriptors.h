#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12RefCounted.h"

#include "Core/Containers/Array.h"
#include "Core/Utilities/StringUtilities.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Platform/CriticalSection.h"

class FD3D12DescriptorHeap;
class FD3D12OnlineDescriptorManager;

typedef TSharedRef<FD3D12DescriptorHeap>          FD3D12DescriptorHeapRef;
typedef TSharedRef<FD3D12OnlineDescriptorManager> FD3D12OnlineDescriptorManagerRef;

class FD3D12DescriptorHeap 
    : public FD3D12DeviceChild
    , public FD3D12RefCounted
{
public:
    FD3D12DescriptorHeap(FD3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32 NumDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS Flags);
    ~FD3D12DescriptorHeap() = default;

    bool Initialize();
    
    FORCEINLINE ID3D12DescriptorHeap*       GetD3D12Heap() const { return Heap.Get(); }

    FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleForHeapStart() const { return CPUStart; }
    FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleForHeapStart() const { return GPUStart; }
    
    FORCEINLINE D3D12_DESCRIPTOR_HEAP_TYPE  GetType() const { return Type; }

    FORCEINLINE uint32                      GetNumDescriptors()                const { return uint32(NumDescriptors); }
    FORCEINLINE uint32                      GetDescriptorHandleIncrementSize() const { return DescriptorHandleIncrementSize; }

    FORCEINLINE void SetName(const FString& Name)
    {
        FStringWide WideName = CharToWide(Name);
        Heap->SetName(WideName.GetCString());
    }

private:
    TComPtr<ID3D12DescriptorHeap> Heap;

    D3D12_CPU_DESCRIPTOR_HANDLE   CPUStart;
    D3D12_GPU_DESCRIPTOR_HANDLE   GPUStart;

    D3D12_DESCRIPTOR_HEAP_TYPE    Type;
    D3D12_DESCRIPTOR_HEAP_FLAGS   Flags;

    uint32                        NumDescriptors;
    uint32                        DescriptorHandleIncrementSize;
};


class FD3D12OfflineDescriptorHeap 
    : public FD3D12DeviceChild
    , public FD3D12RefCounted
{
    struct FDescriptorRange
    {
        FORCEINLINE FDescriptorRange()
            : Begin({ 0 })
            , End({ 0 })
        { }

        FORCEINLINE FDescriptorRange(D3D12_CPU_DESCRIPTOR_HANDLE InBegin, D3D12_CPU_DESCRIPTOR_HANDLE InEnd)
            : Begin(InBegin)
            , End(InEnd)
        { }

        FORCEINLINE bool IsValid() const
        {
            return Begin.ptr < End.ptr;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE Begin = { 0 };
        D3D12_CPU_DESCRIPTOR_HANDLE End   = { 0 };
    };

    struct FDescriptorHeap
    {
        FORCEINLINE FDescriptorHeap(const FD3D12DescriptorHeapRef& InHeap)
            : FreeList()
            , Heap(InHeap)
        {
            FDescriptorRange EntireRange;
            EntireRange.Begin = Heap->GetCPUDescriptorHandleForHeapStart();
            EntireRange.End   = FD3D12CPUDescriptorHandle(EntireRange.Begin, Heap->GetDescriptorHandleIncrementSize(), Heap->GetNumDescriptors());
            FreeList.Emplace(EntireRange);
        }

        FD3D12DescriptorHeapRef  Heap;
        TArray<FDescriptorRange> FreeList;
    };

public:
    FD3D12OfflineDescriptorHeap(FD3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType);
    ~FD3D12OfflineDescriptorHeap() = default;

    bool Initialize();

    D3D12_CPU_DESCRIPTOR_HANDLE Allocate(uint32& OutHeapIndex);
    void Free(D3D12_CPU_DESCRIPTOR_HANDLE Handle, uint32 HeapIndex);

    void SetName(const FString& InName);

    FORCEINLINE uint32                     GetDescriptorSize() const { return DescriptorSize; }
    FORCEINLINE D3D12_DESCRIPTOR_HEAP_TYPE GetType()           const { return Type; }

private:
    bool AllocateHeap();

    FString                    Name;

    TArray<FDescriptorHeap>    Heaps;

    D3D12_DESCRIPTOR_HEAP_TYPE Type;
    uint32                     DescriptorSize = 0;

    FCriticalSection           CriticalSection;
};


class FD3D12OnlineDescriptorHeap 
    : public FD3D12DeviceChild
    , public FD3D12RefCounted
{
public:
    FD3D12OnlineDescriptorHeap(FD3D12Device* InDevice, uint32 InDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE InType);
    ~FD3D12OnlineDescriptorHeap() = default;

    bool Initialize();

    uint32 AllocateHandles(uint32 NumHandles);
    bool   AllocateFreshHeap();

    bool HasSpace(uint32 NumHandles) const;

    void Reset();

    void SetNumPooledHeaps(uint32 NumHeaps);

    FORCEINLINE void SetName(const FString& Name)
    {
        Heap->SetName(Name);
    }

    FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleAt(uint32 Index) const
    {
        return FD3D12CPUDescriptorHandle(Heap->GetCPUDescriptorHandleForHeapStart(), Index, Heap->GetDescriptorHandleIncrementSize());
    }

    FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleAt(uint32 Index) const
    {
        return FD3D12GPUDescriptorHandle(Heap->GetGPUDescriptorHandleForHeapStart(), Index, Heap->GetDescriptorHandleIncrementSize());
    }

    FORCEINLINE uint32 GetDescriptorHandleIncrementSize() const
    {
        return Heap->GetDescriptorHandleIncrementSize();
    }

    FORCEINLINE ID3D12DescriptorHeap* GetD3D12Heap() const { return Heap->GetD3D12Heap(); }
    FORCEINLINE FD3D12DescriptorHeap* GetHeap()      const { return Heap.Get(); }

private:
    FD3D12DescriptorHeapRef Heap;
    uint32                  CurrentHandle   = 0;
    uint32                  DescriptorCount = 0;
    
    TArray<FD3D12DescriptorHeapRef> HeapPool;
    TArray<FD3D12DescriptorHeapRef> DiscardedHeaps;

    D3D12_DESCRIPTOR_HEAP_TYPE      Type;
};


struct FD3D12DescriptorBlock
{
    FD3D12DescriptorBlock()
        : StartDescriptor(0)
        , NumDescriptors(0)
    { }

    FD3D12DescriptorBlock(uint32 InStartDescriptor, uint32 InNumDescriptors)
        : StartDescriptor(InStartDescriptor)
        , NumDescriptors(InNumDescriptors)
    { }

    uint32 StartDescriptor;
    uint32 NumDescriptors;
};


class FD3D12OnlineDescriptorManager 
    : public FD3D12DeviceChild
    , public FD3D12RefCounted
{
public:
    FD3D12OnlineDescriptorManager(FD3D12Device* InDevice, FD3D12DescriptorHeap* InDescriptorHeap, uint32 InDescriptorStartOffset, uint32 InDescriptorCount);
    ~FD3D12OnlineDescriptorManager() = default;

    uint32 AllocateHandles(uint32 NumHandles)
    {
        CHECK(HasSpace(NumHandles));

        const uint32 NewHandle = CurrentHandle;
        CurrentHandle += NumHandles;
        return NewHandle;
    }

    FORCEINLINE bool HasSpace(uint32 NumHandles) const { return ((CurrentHandle + NumHandles) < DescriptorCount); }

    FORCEINLINE void Reset() { CurrentHandle = 0; }

    FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleAt(uint32 Index) const
    {
        CHECK(Index < DescriptorCount);
        return CPUStartHandle.Offset(Index, DescriptorSize); 
    }
    
    FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleAt(uint32 Index) const
    {
        CHECK(Index < DescriptorCount);
        return GPUStartHandle.Offset(Index, DescriptorSize); 
    }

    FORCEINLINE uint32 GetDescriptorHandleIncrementSize() const
    {
        CHECK(DescriptorHeap != nullptr);
        return DescriptorHeap->GetDescriptorHandleIncrementSize();
    }

    FORCEINLINE ID3D12DescriptorHeap* GetD3D12Heap() const
    {
        CHECK(DescriptorHeap != nullptr);
        return DescriptorHeap->GetD3D12Heap();
    }

    FORCEINLINE FD3D12DescriptorHeap* GetHeap() const
    {
        return DescriptorHeap.Get();
    }

private:
    FD3D12DescriptorHeapRef   DescriptorHeap;

    FD3D12CPUDescriptorHandle CPUStartHandle;
    FD3D12GPUDescriptorHandle GPUStartHandle;

    uint32 DescriptorSize        = 0;
    uint32 DescriptorCount       = 0;
    uint32 DescriptorStartOffset = 0;
    uint32 CurrentHandle         = 0;
};