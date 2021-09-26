#pragma once
#include "Core/RefCounted.h"
#include "Core/Containers/Array.h"

#include "Core/Utilities/StringUtilities.h"

#include "D3D12Device.h"
#include "D3D12DeviceChild.h"

class D3D12DescriptorHeap : public D3D12DeviceChild, public CRefCounted
{
public:
    D3D12DescriptorHeap( D3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32 NumDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS Flags );
    ~D3D12DescriptorHeap() = default;

    bool Init();

    void SetName( const std::string& Name )
    {
        WString WideName = CharToWide( CString( Name.c_str(), Name.length() ) );
        Heap->SetName( WideName.CStr() );
    }

    ID3D12DescriptorHeap* GetHeap() const
    {
        return Heap.Get();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleForHeapStart() const
    {
        return CPUStart;
    }
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleForHeapStart() const
    {
        return GPUStart;
    }

    D3D12_DESCRIPTOR_HEAP_TYPE GetType() const
    {
        return Type;
    }

    uint32 GetNumDescriptors() const
    {
        return uint32( NumDescriptors );
    }

    uint32 GetDescriptorHandleIncrementSize() const
    {
        return DescriptorHandleIncrementSize;
    }

private:
    TComPtr<ID3D12DescriptorHeap> Heap;
    D3D12_CPU_DESCRIPTOR_HANDLE   CPUStart;
    D3D12_GPU_DESCRIPTOR_HANDLE   GPUStart;
    D3D12_DESCRIPTOR_HEAP_TYPE    Type;
    D3D12_DESCRIPTOR_HEAP_FLAGS   Flags;
    uint32 NumDescriptors;
    uint32 DescriptorHandleIncrementSize;
};

class D3D12OfflineDescriptorHeap : public D3D12DeviceChild, public CRefCounted
{
    struct DescriptorRange
    {
        DescriptorRange() = default;

        DescriptorRange( D3D12_CPU_DESCRIPTOR_HANDLE InBegin, D3D12_CPU_DESCRIPTOR_HANDLE InEnd )
            : Begin( InBegin )
            , End( InEnd )
        {
        }

        bool IsValid() const
        {
            return Begin.ptr < End.ptr;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE Begin = { 0 };
        D3D12_CPU_DESCRIPTOR_HANDLE End = { 0 };
    };

    struct DescriptorHeap
    {
        DescriptorHeap( const TSharedRef<D3D12DescriptorHeap>& InHeap )
            : FreeList()
            , Heap( InHeap )
        {
            DescriptorRange WholeRange;
            WholeRange.Begin = Heap->GetCPUDescriptorHandleForHeapStart();
            WholeRange.End.ptr = WholeRange.Begin.ptr + (Heap->GetDescriptorHandleIncrementSize() * Heap->GetNumDescriptors());
            FreeList.Emplace( WholeRange );
        }

        TArray<DescriptorRange>   FreeList;
        TSharedRef<D3D12DescriptorHeap> Heap;
    };

public:
    D3D12OfflineDescriptorHeap( D3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType );
    ~D3D12OfflineDescriptorHeap() = default;

    bool Init();

    D3D12_CPU_DESCRIPTOR_HANDLE Allocate( uint32& OutHeapIndex );
    void Free( D3D12_CPU_DESCRIPTOR_HANDLE Handle, uint32 HeapIndex );

    void SetName( const std::string& InName );

    D3D12_DESCRIPTOR_HEAP_TYPE GetType() const
    {
        return Type;
    }

    uint32 GetDescriptorSize() const
    {
        return DescriptorSize;
    }

private:
    bool AllocateHeap();

    D3D12_DESCRIPTOR_HEAP_TYPE Type;
    TArray<DescriptorHeap> Heaps;
    std::string Name;
    uint32 DescriptorSize = 0;
};

class D3D12OnlineDescriptorHeap : public D3D12DeviceChild, public CRefCounted
{
public:
    D3D12OnlineDescriptorHeap( D3D12Device* InDevice, uint32 InDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE InType );
    ~D3D12OnlineDescriptorHeap() = default;

    bool Init();

    uint32 AllocateHandles( uint32 NumHandles );
    bool AllocateFreshHeap();

    bool HasSpace( uint32 NumHandles ) const;

    void Reset();

    void SetName( const std::string& Name )
    {
        Heap->SetName( Name );
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleAt( uint32 Index ) const
    {
        return { Heap->GetCPUDescriptorHandleForHeapStart().ptr + (Index * Heap->GetDescriptorHandleIncrementSize()) };
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleAt( uint32 Index ) const
    {
        return { Heap->GetGPUDescriptorHandleForHeapStart().ptr + (Index * Heap->GetDescriptorHandleIncrementSize()) };
    }

    uint32 GetDescriptorHandleIncrementSize() const
    {
        return Heap->GetDescriptorHandleIncrementSize();
    }

    ID3D12DescriptorHeap* GetNativeHeap() const
    {
        return Heap->GetHeap();
    }
    D3D12DescriptorHeap* GetHeap()       const
    {
        return Heap.Get();
    }

private:
    TSharedRef<D3D12DescriptorHeap> Heap;

    TArray<TSharedRef<D3D12DescriptorHeap>> HeapPool;
    TArray<TSharedRef<D3D12DescriptorHeap>> DiscardedHeaps;

    D3D12_DESCRIPTOR_HEAP_TYPE Type;
    uint32 CurrentHandle = 0;
    uint32 DescriptorCount = 0;
};