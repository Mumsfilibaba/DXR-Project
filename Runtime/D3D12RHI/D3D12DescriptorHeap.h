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

    // The actual heap
    TComPtr<ID3D12DescriptorHeap> Heap;

    // Cached handles for the start of the heap, avoiding virtual calls
    D3D12_CPU_DESCRIPTOR_HANDLE CPUStart;
    D3D12_GPU_DESCRIPTOR_HANDLE GPUStart;

    // Descriptor heap info
    D3D12_DESCRIPTOR_HEAP_TYPE  Type;
    D3D12_DESCRIPTOR_HEAP_FLAGS Flags;

    uint32 NumDescriptors;
    uint32 DescriptorHandleIncrementSize;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class CD3D12OfflineDescriptorHeap : public CD3D12DeviceChild, public CRefCounted
{
    struct SDescriptorRange
    {
        FORCEINLINE SDescriptorRange()
            : Begin( { 0 } )
            , End( { 0 } )
        {
        }

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
            SDescriptorRange EntireRange;
            EntireRange.Begin = Heap->GetCPUDescriptorHandleForHeapStart();
            EntireRange.End = { EntireRange.Begin.ptr + (uint64)(Heap->GetDescriptorHandleIncrementSize() * Heap->GetNumDescriptors()) };
            FreeList.Emplace( EntireRange );
        }

        TSharedRef<CD3D12DescriptorHeap> Heap;
        TArray<SDescriptorRange>         FreeList;
    };

public:

    CD3D12OfflineDescriptorHeap( CD3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType );
    ~CD3D12OfflineDescriptorHeap() = default;

    bool Init();

    /* Allocates new offline handles and returns the Host handle. OutHeapIndex is the Index to the heap the handle belongs to */
    D3D12_CPU_DESCRIPTOR_HANDLE Allocate( uint32& OutHeapIndex );

    /* Frees a offline handle */
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

    // Allocates a new heap and inserts it into the heap-pool 
    bool AllocateHeap();

    // Debug-name
    CString Name;

    // All allocated heaps together with the ranges of available descriptors
    TArray<SDescriptorHeap> Heaps;

    // Heap info
    D3D12_DESCRIPTOR_HEAP_TYPE Type;
    uint32 DescriptorSize = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class CD3D12OnlineDescriptorHeap : public CD3D12DeviceChild, public CRefCounted
{
public:
    CD3D12OnlineDescriptorHeap( CD3D12Device* InDevice, uint32 InDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE InType );
    ~CD3D12OnlineDescriptorHeap() = default;

    bool Init();

    /* Allocates a number of descriptors and returns the first handle to the allocated range */
    uint32 AllocateHandles( uint32 NumHandles );

    /* Allocates a new heap, this means that the heap that is currently bound needs to rebound */
    bool AllocateFreshHeap();

    /* Returns true if the current heap can allocate the required number of descriptors. If false then a fresh heap has to be allocated */
    bool HasSpace( uint32 NumHandles ) const;

    /* Resets the current handle so that new handles will be allocated from the start, and overwrite the current handles */
    void Reset();

    /* Resizes the amount of pooled heaps and deallocates the heaps if the current number is larger than NumHeaps */
    void SetNumPooledHeaps( uint32 NumHeaps );

    FORCEINLINE void SetName( const CString& Name )
    {
        Heap->SetName( Name );
    }

    FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleAt( uint32 Index ) const
    {
        return D3D12_CPU_DESCRIPTOR_HANDLE{ Heap->GetCPUDescriptorHandleForHeapStart().ptr + (Index * Heap->GetDescriptorHandleIncrementSize()) };
    }

    FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleAt( uint32 Index ) const
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

    // The current in use heap 
    TSharedRef<CD3D12DescriptorHeap> Heap;
    // Heaps available to use
    TArray<TSharedRef<CD3D12DescriptorHeap>> HeapPool;
    // Heaps currently in use
    TArray<TSharedRef<CD3D12DescriptorHeap>> DiscardedHeaps;

    D3D12_DESCRIPTOR_HEAP_TYPE Type;
    uint32 CurrentHandle = 0;
    uint32 DescriptorCount = 0;
};