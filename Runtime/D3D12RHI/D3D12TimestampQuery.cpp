#include "D3D12Device.h"
#include "D3D12CommandContext.h"
#include "D3D12TimestampQuery.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHITimestampQuery

CD3D12TimestampQuery::CD3D12TimestampQuery(CD3D12Device* InDevice)
    : CD3D12DeviceChild(InDevice)
    , CRHITimestampQuery()
    , QueryHeap(nullptr)
    , WriteResource(nullptr)
    , ReadResources()
    , TimeQueries()
    , Frequency(0)
{
}

void CD3D12TimestampQuery::GetTimestampFromIndex(SRHITimestamp& OutQuery, uint32 Index) const
{
    if (Index >= (uint32)TimeQueries.Size())
    {
        OutQuery.Begin = 0;
        OutQuery.End = 0;
    }
    else
    {
        OutQuery = TimeQueries[Index];
    }
}

void CD3D12TimestampQuery::BeginQuery(ID3D12GraphicsCommandList* CmdList, uint32 Index)
{
    Check(Index < D3D12_DEFAULT_QUERY_COUNT);
    Check(CmdList != nullptr);

    CmdList->EndQuery(QueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, (Index * 2));

    uint32 QueryCount = Index + 1;
    if (QueryCount >= (uint32)TimeQueries.Size())
    {
        TimeQueries.Resize(QueryCount);
    }
}

void CD3D12TimestampQuery::EndQuery(ID3D12GraphicsCommandList* CmdList, uint32 Index)
{
    Check(CmdList != nullptr);
    Check(Index < (uint32)TimeQueries.Size());

    CmdList->EndQuery(QueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, (Index * 2) + 1);
}

void CD3D12TimestampQuery::ResolveQueries(class CD3D12CommandContext& CmdContext)
{
    CD3D12CommandList          CmdList    = CmdContext.GetCommandList();
    ID3D12CommandQueue*        CmdQueue   = CmdContext.GetQueue().GetQueue();
    ID3D12GraphicsCommandList* GfxCmdList = CmdList.GetGraphicsCommandList();

    Check(CmdQueue != nullptr);
    Check(GfxCmdList != nullptr);

    uint32 ReadIndex = CmdContext.GetCurrentEpochValue();
    if (ReadIndex >= (uint32)ReadResources.Size())
    {
        if (!AllocateReadResource())
        {
            CDebug::DebugBreak();
            return;
        }
    }

    // NOTE: Read the current, the first frames the result will be zero, however this would be expected
    CD3D12Resource* CurrentReadResource = ReadResources[ReadIndex].Get();
    void* Data = CurrentReadResource->Map(0, nullptr);
    if (Data)
    {
        const uint32 SizeInBytes = TimeQueries.SizeInBytes();

        CMemory::Memcpy(TimeQueries.Data(), Data, SizeInBytes);
        CurrentReadResource->Unmap(0, nullptr);
    }

    // Make use of RESOURCE_STATE promotion during the first call
    GfxCmdList->ResolveQueryData(QueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0, D3D12_DEFAULT_QUERY_COUNT, WriteResource->GetResource(), 0);

    CmdList.TransitionBarrier(WriteResource->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
    GfxCmdList->CopyResource(CurrentReadResource->GetResource(), WriteResource->GetResource());
    CmdList.TransitionBarrier(WriteResource->GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

    HRESULT Result = CmdQueue->GetTimestampFrequency(&Frequency);
    if (FAILED(Result))
    {
        LOG_ERROR("[CD3D12TimestampQuery] FAILED to query ClockCalibration");
    }
}

CD3D12TimestampQuery* CD3D12TimestampQuery::Create(CD3D12Device* InDevice)
{
    TSharedRef<CD3D12TimestampQuery> NewProfiler = dbg_new CD3D12TimestampQuery(InDevice);

    ID3D12Device* DxDevice = InDevice->GetD3D12Device();

    D3D12_QUERY_HEAP_DESC QueryHeap;
    CMemory::Memzero(&QueryHeap);

    QueryHeap.Type     = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    QueryHeap.Count    = D3D12_DEFAULT_QUERY_COUNT * 2;
    QueryHeap.NodeMask = 0;

    TComPtr<ID3D12QueryHeap> Heap;
    HRESULT Result = DxDevice->CreateQueryHeap(&QueryHeap, IID_PPV_ARGS(&Heap));
    if (FAILED(Result))
    {
        LOG_ERROR("[D3D12GPUProfiler]: FAILED to create Query Heap");
        return nullptr;
    }

    D3D12_RESOURCE_DESC Desc;
    CMemory::Memzero(&Desc);

    Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    Desc.Flags              = D3D12_RESOURCE_FLAG_NONE;
    Desc.Format             = DXGI_FORMAT_UNKNOWN;
    Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    Desc.Width              = D3D12_DEFAULT_QUERY_COUNT * sizeof(SRHITimestamp);
    Desc.Height             = 1;
    Desc.DepthOrArraySize   = 1;
    Desc.MipLevels          = 1;
    Desc.Alignment          = 0;
    Desc.SampleDesc.Count   = 1;
    Desc.SampleDesc.Quality = 0;

    TSharedRef<CD3D12Resource> WriteResource = dbg_new CD3D12Resource(InDevice, Desc, D3D12_HEAP_TYPE_DEFAULT);
    if (!WriteResource->Init(D3D12_RESOURCE_STATE_COMMON, nullptr))
    {
        return nullptr;
    }
    else
    {
        WriteResource->SetName("Query Write Resource");
    }

    // Start with three
    for (uint32 i = 0; i < D3D12_NUM_BACK_BUFFERS; i++)
    {
        if (!NewProfiler->AllocateReadResource())
        {
            return nullptr;
        }
    }

    NewProfiler->QueryHeap     = Heap;
    NewProfiler->WriteResource = WriteResource;
    return NewProfiler.ReleaseOwnership();
}

bool CD3D12TimestampQuery::AllocateReadResource()
{
    D3D12_RESOURCE_DESC Desc;
    CMemory::Memzero(&Desc);

    Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    Desc.Flags              = D3D12_RESOURCE_FLAG_NONE;
    Desc.Format             = DXGI_FORMAT_UNKNOWN;
    Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    Desc.Width              = D3D12_DEFAULT_QUERY_COUNT * sizeof(SRHITimestamp);
    Desc.Height             = 1;
    Desc.DepthOrArraySize   = 1;
    Desc.MipLevels          = 1;
    Desc.Alignment          = 0;
    Desc.SampleDesc.Count   = 1;
    Desc.SampleDesc.Quality = 0;

    TSharedRef<CD3D12Resource> ReadResource = dbg_new CD3D12Resource(GetDevice(), Desc, D3D12_HEAP_TYPE_READBACK);
    if (ReadResource->Init(D3D12_RESOURCE_STATE_COPY_DEST, nullptr))
    {
        ReadResource->SetName("Query Readback Resource");
        ReadResources.Emplace(ReadResource);
    }
    else
    {
        return false;
    }

    return true;
}
