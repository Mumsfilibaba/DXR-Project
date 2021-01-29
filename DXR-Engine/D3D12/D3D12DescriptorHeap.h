#pragma once
#include <Containers/TArray.h>

#include "Utilities/StringUtilities.h"

#include "Core/RefCountedObject.h"

#include "D3D12Device.h"
#include "D3D12DeviceChild.h"

class D3D12DescriptorHeap : public D3D12DeviceChild, public RefCountedObject
{
public:
    D3D12DescriptorHeap(D3D12Device* InDevice, const D3D12_DESCRIPTOR_HEAP_DESC& InDesc);

    Bool Init();

    FORCEINLINE void SetName(const std::string& Name)
    {
        std::wstring WideName = ConvertToWide(Name);
        Heap->SetName(WideName.c_str());
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
        return Desc.Type;
    }

    FORCEINLINE UInt32 GetNumDescriptors() const
    {
        return UInt32(Desc.NumDescriptors);
    }

    FORCEINLINE UInt32 GetDescriptorHandleIncrementSize() const
    {
        return DescriptorHandleIncrementSize;
    }

private:
    TComPtr<ID3D12DescriptorHeap> Heap;
    D3D12_CPU_DESCRIPTOR_HANDLE   CPUStart;
    D3D12_GPU_DESCRIPTOR_HANDLE   GPUStart;
    D3D12_DESCRIPTOR_HEAP_DESC    Desc;
    UInt32 DescriptorHandleIncrementSize;
};

class D3D12OfflineDescriptorHeap : public D3D12DeviceChild, public RefCountedObject
{
    struct DescriptorRange
    {
        DescriptorRange() = default;

        DescriptorRange(D3D12_CPU_DESCRIPTOR_HANDLE InBegin, D3D12_CPU_DESCRIPTOR_HANDLE InEnd)
            : Begin(InBegin)
            , End(InEnd)
        {
        }

        FORCEINLINE bool IsValid() const
        {
            return Begin.ptr < End.ptr;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE Begin = { 0 };
        D3D12_CPU_DESCRIPTOR_HANDLE End   = { 0 };
    };

    struct DescriptorHeap
    {
        DescriptorHeap(const TSharedRef<D3D12DescriptorHeap>& InHeap)
            : FreeList()
            , Heap(InHeap)
        {
            DescriptorRange WholeRange;
            WholeRange.Begin   = Heap->GetCPUDescriptorHandleForHeapStart();
            WholeRange.End.ptr = WholeRange.Begin.ptr + (Heap->GetDescriptorHandleIncrementSize() * Heap->GetNumDescriptors());
            FreeList.EmplaceBack(WholeRange);
        }

        TArray<DescriptorRange> FreeList;
        TSharedRef<D3D12DescriptorHeap> Heap;
    };

public:
    D3D12OfflineDescriptorHeap(D3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType);

    Bool Init();

    D3D12_CPU_DESCRIPTOR_HANDLE Allocate(UInt32& OutHeapIndex);
    void Free(D3D12_CPU_DESCRIPTOR_HANDLE Handle, UInt32 HeapIndex);

    void SetName(const std::string& InName);

    FORCEINLINE D3D12_DESCRIPTOR_HEAP_TYPE GetType() const
    {
        return Type;
    }

    FORCEINLINE UInt32 GetDescriptorSize() const
    {
        return DescriptorSize;
    }

private:
    Bool AllocateHeap();

    D3D12_DESCRIPTOR_HEAP_TYPE Type;
    TArray<DescriptorHeap> Heaps;
    std::string Name;
    UInt32 DescriptorSize = 0;
};

class D3D12OnlineDescriptorHeap : public D3D12DeviceChild, public RefCountedObject
{
public:
    D3D12OnlineDescriptorHeap(D3D12Device* InDevice, UInt32 InDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE InType);

    Bool Init();

    UInt32 AllocateHandles(UInt32 NumHandles);

    void Reset();

    FORCEINLINE void SetName(const std::string& Name)
    {
        Heap->SetName(Name);
    }

    FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleAt(UInt32 Index) const
    {
        return { Heap->GetCPUDescriptorHandleForHeapStart().ptr + (Index * Heap->GetDescriptorHandleIncrementSize()) };
    }

    FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleAt(UInt32 Index) const
    {
        return { Heap->GetGPUDescriptorHandleForHeapStart().ptr + (Index * Heap->GetDescriptorHandleIncrementSize()) };
    }

    FORCEINLINE UInt32 GetDescriptorHandleIncrementSize() const
    {
        return Heap->GetDescriptorHandleIncrementSize();
    }
    
    FORCEINLINE ID3D12DescriptorHeap* GetNativeHeap() const
    {
        return Heap->GetHeap();
    }

    FORCEINLINE D3D12DescriptorHeap* GetHeap() const
    {
        return Heap.Get();
    }

private:
    TSharedRef<D3D12DescriptorHeap> Heap;
    TArray<TSharedRef<D3D12DescriptorHeap>> HeapPool;
    TArray<TSharedRef<D3D12DescriptorHeap>> DiscardedHeaps;
    
    D3D12_DESCRIPTOR_HEAP_TYPE Type;
    UInt32 CurrentHandle   = 0;
    UInt32 DescriptorCount = 0;
};