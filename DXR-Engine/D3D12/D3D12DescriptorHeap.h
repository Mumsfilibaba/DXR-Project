#pragma once
#include <Containers/Array.h>

#include "Utilities/StringUtilities.h"

#include "Core/RefCountedObject.h"

#include "D3D12Device.h"
#include "D3D12DeviceChild.h"

class D3D12DescriptorHeap : public D3D12DeviceChild, public RefCountedObject
{
public:
    D3D12DescriptorHeap(D3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE Type, UInt32 NumDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS Flags);
    ~D3D12DescriptorHeap() = default;

    Bool Init();

    void SetName(const std::string& Name)
    {
        std::wstring WideName = ConvertToWide(Name);
        Heap->SetName(WideName.c_str());
    }

    ID3D12DescriptorHeap* GetHeap() const { return Heap.Get(); }

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleForHeapStart() const { return CPUStart; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleForHeapStart() const { return GPUStart; }

    D3D12_DESCRIPTOR_HEAP_TYPE GetType() const { return Type; }

    UInt32 GetNumDescriptors() const { return UInt32(NumDescriptors); }

    UInt32 GetDescriptorHandleIncrementSize() const { return DescriptorHandleIncrementSize; }

private:
    TComPtr<ID3D12DescriptorHeap> Heap;
    D3D12_CPU_DESCRIPTOR_HANDLE   CPUStart;
    D3D12_GPU_DESCRIPTOR_HANDLE   GPUStart;
    D3D12_DESCRIPTOR_HEAP_TYPE    Type;
    D3D12_DESCRIPTOR_HEAP_FLAGS   Flags;
    UInt32 NumDescriptors; 
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

        Bool IsValid() const
        {
            return Begin.ptr < End.ptr;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE Begin = { 0 };
        D3D12_CPU_DESCRIPTOR_HANDLE End   = { 0 };
    };

    struct DescriptorHeap
    {
        DescriptorHeap(const TRef<D3D12DescriptorHeap>& InHeap)
            : FreeList()
            , Heap(InHeap)
        {
            DescriptorRange WholeRange;
            WholeRange.Begin   = Heap->GetCPUDescriptorHandleForHeapStart();
            WholeRange.End.ptr = WholeRange.Begin.ptr + (Heap->GetDescriptorHandleIncrementSize() * Heap->GetNumDescriptors());
            FreeList.EmplaceBack(WholeRange);
        }

        TArray<DescriptorRange>   FreeList;
        TRef<D3D12DescriptorHeap> Heap;
    };

public:
    D3D12OfflineDescriptorHeap(D3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType);
    ~D3D12OfflineDescriptorHeap() = default;

    Bool Init();

    D3D12_CPU_DESCRIPTOR_HANDLE Allocate(UInt32& OutHeapIndex);
    void Free(D3D12_CPU_DESCRIPTOR_HANDLE Handle, UInt32 HeapIndex);

    void SetName(const std::string& InName);

    D3D12_DESCRIPTOR_HEAP_TYPE GetType() const
    {
        return Type;
    }

    UInt32 GetDescriptorSize() const
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
    ~D3D12OnlineDescriptorHeap() = default;

    Bool Init();

    UInt32 AllocateHandles(UInt32 NumHandles);

    void Reset();

    void SetName(const std::string& Name)
    {
        Heap->SetName(Name);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleAt(UInt32 Index) const
    {
        return { Heap->GetCPUDescriptorHandleForHeapStart().ptr + (Index * Heap->GetDescriptorHandleIncrementSize()) };
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleAt(UInt32 Index) const
    {
        return { Heap->GetGPUDescriptorHandleForHeapStart().ptr + (Index * Heap->GetDescriptorHandleIncrementSize()) };
    }

    UInt32 GetDescriptorHandleIncrementSize() const
    {
        return Heap->GetDescriptorHandleIncrementSize();
    }
    
    ID3D12DescriptorHeap* GetNativeHeap() const { return Heap->GetHeap(); }
    D3D12DescriptorHeap*  GetHeap()       const { return Heap.Get(); }

private:
    TRef<D3D12DescriptorHeap> Heap;

    TArray<TRef<D3D12DescriptorHeap>> HeapPool;
    TArray<TRef<D3D12DescriptorHeap>> DiscardedHeaps;
    
    D3D12_DESCRIPTOR_HEAP_TYPE Type;
    UInt32 CurrentHandle   = 0;
    UInt32 DescriptorCount = 0;
};