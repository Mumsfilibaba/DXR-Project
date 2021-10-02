#pragma once
#include "Core/RefCounted.h"
#include "Core/Containers/Array.h"

#include "Core/Utilities/StringUtilities.h"

#include "D3D12Device.h"
#include "D3D12DeviceChild.h"

class CD3D12DescriptorHeap : public CD3D12DeviceChild, public CRefCounted
{
public:
    CD3D12DescriptorHeap( CD3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32 NumDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS Flags );
    ~CD3D12DescriptorHeap() = default;

    bool Init();

    FORCEINLINE void SetName( const CString& Name )
    {
        WString WideName = CharToWide( Name );
        Heap->SetName( WideName.CStr() );
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
        return uint32( NumDescriptors );
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

class CD3D12OfflineDescriptorHeap : public CD3D12DeviceChild, public CRefCounted
{
    struct SDescriptorRange
    {
        SDescriptorRange() = default;

        FORCEINLINE SDescriptorRange( D3D12_CPU_DESCRIPTOR_HANDLE InBegin, D3D12_CPU_DESCRIPTOR_HANDLE InEnd )
            : Begin( InBegin )
            , End( InEnd )
        {
        }

        FORCEINLINE bool IsValid() const
        {
            return Begin.ptr < End.ptr;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE Begin = { 0 };
        D3D12_CPU_DESCRIPTOR_HANDLE End = { 0 };
    };

    struct SDescriptorHeap
    {
        FORCEINLINE SDescriptorHeap( const TSharedRef<CD3D12DescriptorHeap>& InHeap )
            : FreeList()
            , Heap( InHeap )
        {
            SDescriptorRange WholeRange;
            WholeRange.Begin = Heap->GetCPUDescriptorHandleForHeapStart();
            WholeRange.End.ptr = WholeRange.Begin.ptr + (Heap->GetDescriptorHandleIncrementSize() * Heap->GetNumDescriptors());
            FreeList.Emplace( WholeRange );
        }

        TArray<SDescriptorRange>   FreeList;
        TSharedRef<CD3D12DescriptorHeap> Heap;
    };

public:
    CD3D12OfflineDescriptorHeap( CD3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType );
    ~CD3D12OfflineDescriptorHeap() = default;

    bool Init();

    D3D12_CPU_DESCRIPTOR_HANDLE Allocate( uint32& OutHeapIndex );
    void Free( D3D12_CPU_DESCRIPTOR_HANDLE Handle, uint32 HeapIndex );

    void SetName( const CString& InName );

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

    CString Name;

    TArray<SDescriptorHeap> Heaps;

    D3D12_DESCRIPTOR_HEAP_TYPE Type;
    uint32 DescriptorSize = 0;
};

class CD3D12OnlineDescriptorHeap : public CD3D12DeviceChild, public CRefCounted
{
public:
    CD3D12OnlineDescriptorHeap( CD3D12Device* InDevice, uint32 InDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE InType );
    ~CD3D12OnlineDescriptorHeap() = default;

    bool Init();

    uint32 AllocateHandles( uint32 NumHandles );
    bool AllocateFreshHeap();

    bool HasSpace( uint32 NumHandles ) const;

    void Reset();

    FORCEINLINE void SetName( const CString& Name )
    {
        Heap->SetName( Name );
    }

    FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleAt( uint32 Index ) const
    {
        return { Heap->GetCPUDescriptorHandleForHeapStart().ptr + (Index * Heap->GetDescriptorHandleIncrementSize()) };
    }

    FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleAt( uint32 Index ) const
    {
        return { Heap->GetGPUDescriptorHandleForHeapStart().ptr + (Index * Heap->GetDescriptorHandleIncrementSize()) };
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

    TArray<TSharedRef<CD3D12DescriptorHeap>> HeapPool;
    TArray<TSharedRef<CD3D12DescriptorHeap>> DiscardedHeaps;

    D3D12_DESCRIPTOR_HEAP_TYPE Type;
    uint32 CurrentHandle = 0;
    uint32 DescriptorCount = 0;
};