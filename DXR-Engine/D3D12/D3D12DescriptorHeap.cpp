#include "D3D12DescriptorHeap.h"
#include "D3D12Device.h"
#include "D3D12Views.h"

D3D12DescriptorHeap::D3D12DescriptorHeap( D3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType, uint32 InNumDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS InFlags )
    : D3D12DeviceChild( InDevice )
    , Heap( nullptr )
    , CPUStart( { 0 } )
    , GPUStart( { 0 } )
    , DescriptorHandleIncrementSize( 0 )
    , Type( InType )
    , NumDescriptors( InNumDescriptors )
    , Flags( InFlags )
{
}

bool D3D12DescriptorHeap::Init()
{
    D3D12_DESCRIPTOR_HEAP_DESC Desc;
    Memory::Memzero( &Desc );

    Desc.Type = Type;
    Desc.Flags = Flags;
    Desc.NumDescriptors = NumDescriptors;

    HRESULT Result = GetDevice()->GetDevice()->CreateDescriptorHeap( &Desc, IID_PPV_ARGS( &Heap ) );
    if ( FAILED( Result ) )
    {
        LOG_ERROR( "[D3D12DescriptorHeap]: FAILED to Create DescriptorHeap" );
        Debug::DebugBreak();

        return false;
    }
    else
    {
        LOG_INFO( "[D3D12DescriptorHeap]: Created DescriptorHeap" );
    }

    CPUStart = Heap->GetCPUDescriptorHandleForHeapStart();
    GPUStart = Heap->GetGPUDescriptorHandleForHeapStart();
    DescriptorHandleIncrementSize = GetDevice()->GetDescriptorHandleIncrementSize( Desc.Type );

    return true;
}

D3D12OfflineDescriptorHeap::D3D12OfflineDescriptorHeap( D3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE InType )
    : D3D12DeviceChild( InDevice )
    , Heaps()
    , Name()
    , Type( InType )
{
}

bool D3D12OfflineDescriptorHeap::Init()
{
    DescriptorSize = GetDevice()->GetDescriptorHandleIncrementSize( Type );
    return AllocateHeap();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12OfflineDescriptorHeap::Allocate( uint32& OutHeapIndex )
{
    // Find a heap that is not empty
    uint32 HeapIndex = 0;
    bool FoundHeap = false;
    for ( DescriptorHeap& Heap : Heaps )
    {
        if ( !Heap.FreeList.IsEmpty() )
        {
            FoundHeap = true;
            break;
        }
        else
        {
            HeapIndex++;
        }
    }

    if ( !FoundHeap )
    {
        if ( !AllocateHeap() )
        {
            return { 0 };
        }

        HeapIndex = static_cast<uint32>(Heaps.Size()) - 1;
    }

    // Get the heap and the first free range
    DescriptorHeap& Heap = Heaps[HeapIndex];
    DescriptorRange& Range = Heap.FreeList.Front();

    D3D12_CPU_DESCRIPTOR_HANDLE Handle = Range.Begin;
    Range.Begin.ptr += DescriptorSize;

    if ( !Range.IsValid() )
    {
        Heap.FreeList.Erase( Heap.FreeList.Begin() );
    }

    OutHeapIndex = HeapIndex;
    return Handle;
}

void D3D12OfflineDescriptorHeap::Free( D3D12_CPU_DESCRIPTOR_HANDLE Handle, uint32 HeapIndex )
{
    Assert( HeapIndex < Heaps.Size() );
    DescriptorHeap& Heap = Heaps[HeapIndex];

    // Find a suitable range
    bool FoundRange = false;
    for ( DescriptorRange& Range : Heap.FreeList )
    {
        Assert( Range.IsValid() );

        if ( Handle.ptr + DescriptorSize == Range.Begin.ptr )
        {
            Range.Begin = Handle;
            FoundRange = true;

            break;
        }
        else if ( Handle.ptr == Range.End.ptr )
        {
            Range.End.ptr += DescriptorSize;
            FoundRange = true;

            break;
        }
    }

    if ( !FoundRange )
    {
        D3D12_CPU_DESCRIPTOR_HANDLE End = { Handle.ptr + DescriptorSize };
        Heap.FreeList.EmplaceBack( Handle, End );
    }
}

void D3D12OfflineDescriptorHeap::SetName( const std::string& InName )
{
    Name = InName;

    uint32 HeapIndex = 0;
    for ( DescriptorHeap& Heap : Heaps )
    {
        std::string DbgName = Name + "[" + std::to_string( HeapIndex ) + "]";
        Heap.Heap->SetName( DbgName.c_str() );
    }
}

bool D3D12OfflineDescriptorHeap::AllocateHeap()
{
    constexpr uint32 DescriptorCount = D3D12_MAX_OFFLINE_DESCRIPTOR_COUNT;

    TRef<D3D12DescriptorHeap> Heap = DBG_NEW D3D12DescriptorHeap( GetDevice(), Type, DescriptorCount, D3D12_DESCRIPTOR_HEAP_FLAG_NONE );
    if ( Heap->Init() )
    {
        if ( !Name.empty() )
        {
            std::string DbgName = Name + std::to_string( Heaps.Size() );
            Heap->SetName( DbgName.c_str() );
        }

        Heaps.EmplaceBack( Heap );
        return true;
    }
    else
    {
        return false;
    }
}

D3D12OnlineDescriptorHeap::D3D12OnlineDescriptorHeap( D3D12Device* InDevice, uint32 InDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE InType )
    : D3D12DeviceChild( InDevice )
    , Heap( nullptr )
    , DescriptorCount( InDescriptorCount )
    , Type( InType )
{
}

bool D3D12OnlineDescriptorHeap::Init()
{
    Heap = DBG_NEW D3D12DescriptorHeap( GetDevice(), Type, DescriptorCount, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE );
    if ( Heap->Init() )
    {
        return true;
    }
    else
    {
        return false;
    }
}

void D3D12OnlineDescriptorHeap::Reset()
{
    if ( !HeapPool.IsEmpty() )
    {
        for ( TRef<D3D12DescriptorHeap>& CurrentHeap : DiscardedHeaps )
        {
            HeapPool.EmplaceBack( CurrentHeap );
        }

        DiscardedHeaps.Clear();
    }
    else
    {
        HeapPool.Swap( DiscardedHeaps );
    }

    CurrentHandle = 0;
}

uint32 D3D12OnlineDescriptorHeap::AllocateHandles( uint32 NumHandles )
{
    Assert( NumHandles <= DescriptorCount );

    if ( !HasSpace( NumHandles ) )
    {
        if ( !AllocateFreshHeap() )
        {
            return (uint32)-1;
        }
    }

    const uint32 Handle = CurrentHandle;
    CurrentHandle += NumHandles;
    return Handle;
}

bool D3D12OnlineDescriptorHeap::AllocateFreshHeap()
{
    DiscardedHeaps.EmplaceBack( Heap );

    if ( HeapPool.IsEmpty() )
    {
        Heap = DBG_NEW D3D12DescriptorHeap( GetDevice(), Type, DescriptorCount, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE );
        if ( !Heap->Init() )
        {
            Debug::DebugBreak();
            return false;
        }
    }
    else
    {
        Heap = HeapPool.Back();
        HeapPool.PopBack();
    }

    CurrentHandle = 0;
    return true;
}

bool D3D12OnlineDescriptorHeap::HasSpace( uint32 NumHandles ) const
{
    const uint32 NewCurrentHandle = CurrentHandle + NumHandles;
    return NewCurrentHandle < DescriptorCount;
}
