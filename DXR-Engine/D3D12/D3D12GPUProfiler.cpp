#include "D3D12GPUProfiler.h"
#include "D3D12Device.h"
#include "D3D12CommandContext.h"

D3D12GPUProfiler::D3D12GPUProfiler( D3D12Device* InDevice )
    : D3D12DeviceChild( InDevice )
    , GPUProfiler()
    , QueryHeap( nullptr )
    , WriteResource( nullptr )
    , ReadResources()
    , TimeQueries()
    , Frequency( 0 )
{
}

void D3D12GPUProfiler::GetTimeQuery( TimeQuery& OutQuery, uint32 Index ) const
{
    if ( Index >= TimeQueries.Size() )
    {
        OutQuery.Begin = 0;
        OutQuery.End = 0;
    }
    else
    {
        OutQuery = TimeQueries[Index];
    }
}

void D3D12GPUProfiler::BeginQuery( ID3D12GraphicsCommandList* CmdList, uint32 Index )
{
    Assert( Index < D3D12_DEFAULT_QUERY_COUNT );
    Assert( CmdList != nullptr );

    CmdList->EndQuery( QueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, (Index * 2) );

    uint32 QueryCount = Index + 1;
    if ( QueryCount >= TimeQueries.Size() )
    {
        TimeQueries.Resize( QueryCount );
    }
}

void D3D12GPUProfiler::EndQuery( ID3D12GraphicsCommandList* CmdList, uint32 Index )
{
    Assert( CmdList != nullptr );
    Assert( Index < TimeQueries.Size() );

    CmdList->EndQuery( QueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, (Index * 2) + 1 );
}

void D3D12GPUProfiler::ResolveQueries( class D3D12CommandContext& CmdContext )
{
    D3D12CommandListHandle CmdList = CmdContext.GetCommandList();
    ID3D12CommandQueue* CmdQueue = CmdContext.GetQueue().GetQueue();
    ID3D12GraphicsCommandList* GfxCmdList = CmdList.GetGraphicsCommandList();

    Assert( CmdQueue != nullptr );
    Assert( GfxCmdList != nullptr );

    uint32 ReadIndex = CmdContext.GetCurrentEpochValue();
    if ( ReadIndex >= ReadResources.Size() )
    {
        if ( !AllocateReadResource() )
        {
            Debug::DebugBreak();
            return;
        }
    }

    // NOTE: Read the current, the first frames the result will be zero, howver this would be expected
    D3D12Resource* CurrentReadResource = ReadResources[ReadIndex].Get();
    void* Data = CurrentReadResource->Map( 0, nullptr );
    if ( Data )
    {
        const uint32 SizeInBytes = TimeQueries.SizeInBytes();

        Memory::Memcpy( TimeQueries.Data(), Data, SizeInBytes );
        CurrentReadResource->Unmap( 0, nullptr );
    }

    // Make use of RESOURCE_STATE promotion during the first call
    GfxCmdList->ResolveQueryData( QueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0, D3D12_DEFAULT_QUERY_COUNT, WriteResource->GetResource(), 0 );

    CmdList.TransitionBarrier( WriteResource->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES );
    GfxCmdList->CopyResource( CurrentReadResource->GetResource(), WriteResource->GetResource() );
    CmdList.TransitionBarrier( WriteResource->GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES );

    HRESULT Result = CmdQueue->GetTimestampFrequency( &Frequency );
    if ( FAILED( Result ) )
    {
        LOG_ERROR( "[D3D12GPUProfiler] FAILED to query ClockCalibration" );
    }
}

D3D12GPUProfiler* D3D12GPUProfiler::Create( D3D12Device* InDevice )
{
    TRef<D3D12GPUProfiler> NewProfiler = DBG_NEW D3D12GPUProfiler( InDevice );

    ID3D12Device* DxDevice = InDevice->GetDevice();

    D3D12_QUERY_HEAP_DESC QueryHeap;
    Memory::Memzero( &QueryHeap );

    QueryHeap.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    QueryHeap.Count = D3D12_DEFAULT_QUERY_COUNT * 2;
    QueryHeap.NodeMask = 0;

    TComPtr<ID3D12QueryHeap> Heap;
    HRESULT Result = DxDevice->CreateQueryHeap( &QueryHeap, IID_PPV_ARGS( &Heap ) );
    if ( FAILED( Result ) )
    {
        LOG_ERROR( "[D3D12GPUProfiler]: FAILED to create Query Heap" );
        return nullptr;
    }

    D3D12_RESOURCE_DESC Desc;
    Memory::Memzero( &Desc );

    Desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    Desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    Desc.Format = DXGI_FORMAT_UNKNOWN;
    Desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    Desc.Width = D3D12_DEFAULT_QUERY_COUNT * sizeof( TimeQuery );
    Desc.Height = 1;
    Desc.DepthOrArraySize = 1;
    Desc.MipLevels = 1;
    Desc.Alignment = 0;
    Desc.SampleDesc.Count = 1;
    Desc.SampleDesc.Quality = 0;

    TRef<D3D12Resource> WriteResource = DBG_NEW D3D12Resource( InDevice, Desc, D3D12_HEAP_TYPE_DEFAULT );
    if ( !WriteResource->Init( D3D12_RESOURCE_STATE_COMMON, nullptr ) )
    {
        return nullptr;
    }
    else
    {
        WriteResource->SetName( "Query Write Resource" );
    }

    // Start with three
    for ( uint32 i = 0; i < D3D12_NUM_BACK_BUFFERS; i++ )
    {
        if ( !NewProfiler->AllocateReadResource() )
        {
            return nullptr;
        }
    }

    NewProfiler->QueryHeap = Heap;
    NewProfiler->WriteResource = WriteResource;
    return NewProfiler.ReleaseOwnership();
}

bool D3D12GPUProfiler::AllocateReadResource()
{
    D3D12_RESOURCE_DESC Desc;
    Memory::Memzero( &Desc );

    Desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    Desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    Desc.Format = DXGI_FORMAT_UNKNOWN;
    Desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    Desc.Width = D3D12_DEFAULT_QUERY_COUNT * sizeof( TimeQuery );
    Desc.Height = 1;
    Desc.DepthOrArraySize = 1;
    Desc.MipLevels = 1;
    Desc.Alignment = 0;
    Desc.SampleDesc.Count = 1;
    Desc.SampleDesc.Quality = 0;

    TRef<D3D12Resource> ReadResource = DBG_NEW D3D12Resource( GetDevice(), Desc, D3D12_HEAP_TYPE_READBACK );
    if ( ReadResource->Init( D3D12_RESOURCE_STATE_COPY_DEST, nullptr ) )
    {
        ReadResource->SetName( "Query Readback Resource" );
        ReadResources.EmplaceBack( ReadResource );
    }
    else
    {
        return false;
    }

    return true;
}
