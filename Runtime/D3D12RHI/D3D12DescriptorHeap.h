#pragma once
#include "Core/RefCounted.h"
#include "Core/Containers/Array.h"

#include "Core/Utilities/StringUtilities.h"

#include "D3D12Device.h"
#include "D3D12DeviceChild.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12DescriptorHeap

class CD3D12DescriptorHeap : public CD3D12DeviceChild, public CRefCounted
{
public:

    CD3D12DescriptorHeap(FD3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32 NumDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS Flags);
    ~CD3D12DescriptorHeap() = default;

    bool Init();

    FORCEINLINE void SetName(const String& Name)
    {
        WString WideName = CharToWide(Name);
        Heap->SetName(WideName.CStr());
    }

    FORCEINLINE ID3D12DescriptorHeap* GetHeap() const
    {
        return Heap.Get();
    }

    FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleForHeapStart() const
    {
        return CPUStart;
    }

    FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleForHeapStart() const
    {
        return GPUStart;
    }

    FORCEINLINE D3D12_DESCRIPTOR_HEAP_TYPE GetType() const
    {
        return Type;
    }

    FORCEINLINE uint32 GetNumDescriptors() const
    {
        return uint32(NumDescriptors);
    }

    FORCEINLINE uint32 GetDescriptorHandleIncrementSize() const
    {
        return DescriptorHandleIncrementSize;
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
// D3D12OfflineDescriptorHeap

class CD3D12OfflineDescriptorHeap : public CD3D12DeviceChild, public CRefCounted
{
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // SDescriptorRange

    struct SDescriptorRange
    {
        FORCEINLINE SDescriptorRange()
            : Begin({ 0 })
            , End({ 0 })
        {
        }

        FORCEINLINE SDescriptorRange(D3D12_CPU_DESCRIPTOR_HANDLE InBegin, D3D12_CPU_DESCRIPTOR_HANDLE InEnd)
            : Begin(InBegin)
            , End(InEnd)
        {
        }

        FORCEINLINE bool IsValid() const
        {
            return Begin.ptr < End.ptr;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE Begin = { 0 };
        D3D12_CPU_DESCRIPTOR_HANDLE End = { 0 };
    };

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // SDescriptorHeap

    struct SDescriptorHeap
    {
        FORCEINLINE SDescriptorHeap(const TSharedRef<CD3D12DescriptorHeap>& InHeap)
            : FreeList()
            , Heap(InHeap)
        {
            SDescriptorRange EntireRange;
            EntireRange.Begin = Heap->GetCPUDescriptorHandleForHeapStart();
            EntireRange.End = { EntireRange.Begin.ptr + (uint64)(Heap->GetDescriptorHandleIncrementSize() * Heap->GetNumDescriptors()) };
            FreeList.Emplace(EntireRange);
        }

        TSharedRef<CD3D12DescriptorHeap> Heap;
        TArray<SDescriptorRange>         FreeList;
    };

public:

    CD3D12OfflineDescriptorHeap(FD3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType);
    ~CD3D12OfflineDescriptorHeap() = default;

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
// D3D12OnlineDescriptorHeap

class CD3D12OnlineDescriptorHeap : public CD3D12DeviceChild, public CRefCounted
{
public:
    CD3D12OnlineDescriptorHeap(FD3D12Device* InDevice, uint32 InDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE InType);
    ~CD3D12OnlineDescriptorHeap() = default;

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

    FORCEINLINE CD3D12DescriptorHeap* GetHeap() const
    {
        return Heap.Get();
    }

private:
    TSharedRef<CD3D12DescriptorHeap> Heap;
    uint32 CurrentHandle   = 0;
    uint32 DescriptorCount = 0;
    
    TArray<TSharedRef<CD3D12DescriptorHeap>> HeapPool;
    TArray<TSharedRef<CD3D12DescriptorHeap>> DiscardedHeaps;

    D3D12_DESCRIPTOR_HEAP_TYPE Type;
};