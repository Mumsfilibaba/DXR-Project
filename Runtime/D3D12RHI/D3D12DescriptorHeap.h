#pragma once
#include "D3D12Device.h"
#include "D3D12DeviceChild.h"
#include "D3D12RefCounted.h"

#include "Core/Containers/Array.h"
#include "Core/Utilities/StringUtilities.h"

class FD3D12DescriptorHeap;

typedef TSharedRef<FD3D12DescriptorHeap> FD3D12DescriptorHeapRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12DescriptorHeap

class FD3D12DescriptorHeap : public FD3D12DeviceChild, public FD3D12RefCounted
{
public:

    FD3D12DescriptorHeap(FD3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32 NumDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS Flags);
    ~FD3D12DescriptorHeap() = default;

    bool Initialize();
    
public:

    ID3D12DescriptorHeap*       GetD3D12Heap() const { return Heap.Get(); }

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleForHeapStart() const { return CPUStart; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleForHeapStart() const { return GPUStart; }
    
    D3D12_DESCRIPTOR_HEAP_TYPE  GetType() const { return Type; }

    uint32                      GetNumDescriptors() const { return uint32(NumDescriptors); }
    uint32                      GetDescriptorHandleIncrementSize() const { return DescriptorHandleIncrementSize; }

    FORCEINLINE void SetName(const String& Name)
    {
        WString WideName = CharToWide(Name);
        Heap->SetName(WideName.CStr());
    }

private:
    TComPtr<ID3D12DescriptorHeap> Heap;

    D3D12_CPU_DESCRIPTOR_HANDLE CPUStart;
    D3D12_GPU_DESCRIPTOR_HANDLE GPUStart;

    D3D12_DESCRIPTOR_HEAP_TYPE  Type;
    D3D12_DESCRIPTOR_HEAP_FLAGS Flags;

    uint32 NumDescriptors;
    uint32 DescriptorHandleIncrementSize;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12OfflineDescriptorHeap

class FD3D12OfflineDescriptorHeap : public FD3D12DeviceChild, public FD3D12RefCounted
{
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // SDescriptorRange

    struct SDescriptorRange
    {
        FORCEINLINE SDescriptorRange()
            : Begin({ 0 })
            , End({ 0 })
        { }

        FORCEINLINE SDescriptorRange(D3D12_CPU_DESCRIPTOR_HANDLE InBegin, D3D12_CPU_DESCRIPTOR_HANDLE InEnd)
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

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // SDescriptorHeap

    struct SDescriptorHeap
    {
        FORCEINLINE SDescriptorHeap(const FD3D12DescriptorHeapRef& InHeap)
            : FreeList()
            , Heap(InHeap)
        {
            SDescriptorRange EntireRange;
            EntireRange.Begin = Heap->GetCPUDescriptorHandleForHeapStart();
            EntireRange.End = { EntireRange.Begin.ptr + (uint64)(Heap->GetDescriptorHandleIncrementSize() * Heap->GetNumDescriptors()) };
            FreeList.Emplace(EntireRange);
        }

        FD3D12DescriptorHeapRef  Heap;
        TArray<SDescriptorRange> FreeList;
    };

public:

    FD3D12OfflineDescriptorHeap(FD3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType);
    ~FD3D12OfflineDescriptorHeap() = default;

    bool Init();

    D3D12_CPU_DESCRIPTOR_HANDLE Allocate(uint32& OutHeapIndex);

    void Free(D3D12_CPU_DESCRIPTOR_HANDLE Handle, uint32 HeapIndex);

    void SetName(const String& InName);

    FORCEINLINE D3D12_DESCRIPTOR_HEAP_TYPE GetType() const
    {
        return Type;
    }

    FORCEINLINE uint32 GetDescriptorSize() const
    {
        return DescriptorSize;
    }

private:
    bool AllocateHeap();

    String Name;

    TArray<SDescriptorHeap> Heaps;

    D3D12_DESCRIPTOR_HEAP_TYPE Type;
    uint32 DescriptorSize = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12OnlineDescriptorHeap

class FD3D12OnlineDescriptorHeap : public FD3D12DeviceChild, public FD3D12RefCounted
{
public:
    FD3D12OnlineDescriptorHeap(FD3D12Device* InDevice, uint32 InDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE InType);
    ~FD3D12OnlineDescriptorHeap() = default;

    bool Init();

    uint32 AllocateHandles(uint32 NumHandles);
    bool AllocateFreshHeap();

    bool HasSpace(uint32 NumHandles) const;

    void Reset();

    void SetNumPooledHeaps(uint32 NumHeaps);

    FORCEINLINE void SetName(const String& Name)
    {
        Heap->SetName(Name);
    }

    FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleAt(uint32 Index) const
    {
        return D3D12_CPU_DESCRIPTOR_HANDLE{ Heap->GetCPUDescriptorHandleForHeapStart().ptr + (Index * Heap->GetDescriptorHandleIncrementSize()) };
    }

    FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleAt(uint32 Index) const
    {
        return D3D12_GPU_DESCRIPTOR_HANDLE{ Heap->GetGPUDescriptorHandleForHeapStart().ptr + (Index * Heap->GetDescriptorHandleIncrementSize()) };
    }

    FORCEINLINE uint32 GetDescriptorHandleIncrementSize() const
    {
        return Heap->GetDescriptorHandleIncrementSize();
    }

    FORCEINLINE ID3D12DescriptorHeap* GetNativeHeap() const
    {
        return Heap->GetHeap();
    }

    FORCEINLINE FD3D12DescriptorHeap* GetHeap() const
    {
        return Heap.Get();
    }

private:
    FD3D12DescriptorHeapRef Heap;
    uint32 CurrentHandle   = 0;
    uint32 DescriptorCount = 0;
    
    TArray<FD3D12DescriptorHeapRef> HeapPool;
    TArray<FD3D12DescriptorHeapRef> DiscardedHeaps;

    D3D12_DESCRIPTOR_HEAP_TYPE Type;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12OnlineDescriptorManager

