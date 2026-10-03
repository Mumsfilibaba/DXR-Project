#include "Core/Platform/PlatformThreadMisc.h"
#include "D3D11RHI/D3D11Query.h"
#include "D3D11RHI/D3D11Device.h"
#include "D3D11RHI/D3D11Stats.h"

static D3D11_QUERY ConvertQueryType(EQueryType QueryType)
{
    switch (QueryType)
    {
        case EQueryType::Timestamp:          return D3D11_QUERY_TIMESTAMP;
        case EQueryType::Occlusion:          return D3D11_QUERY_OCCLUSION;
        case EQueryType::PipelineStatistics: return D3D11_QUERY_PIPELINE_STATISTICS;
        default:                             return D3D11_QUERY_EVENT;
    }
}

static uint64 ConvertTicksToNanoseconds(uint64 Ticks, uint64 Frequency)
{
    constexpr uint64 NanosecondsPerSecond = 1000ull * 1000ull * 1000ull;

    // GPU clocks count from boot, so Ticks * NanosecondsPerSecond would overflow
    const uint64 Seconds   = Ticks / Frequency;
    const uint64 Remainder = Ticks % Frequency;
    return Seconds * NanosecondsPerSecond + (Remainder * NanosecondsPerSecond) / Frequency;
}

FD3D11Query::FD3D11Query(FD3D11Device* InDevice, D3D11_QUERY InQueryType)
    : FD3D11DeviceChild(InDevice)
    , FRefCountedBase()
    , Query(nullptr)
    , QueryType(InQueryType)
    , bIsIssued(0)
{
}

FD3D11Query::~FD3D11Query()
{
    if (Query)
    {
        STAT_SUBTRACT(STAT_D3D11_QueryCount, 1);
    }
}

bool FD3D11Query::Initialize()
{
    D3D11_QUERY_DESC QueryDesc = {};
    QueryDesc.Query     = QueryType;
    QueryDesc.MiscFlags = 0;

    HRESULT Result = GetDevice()->GetD3D11Device()->CreateQuery(&QueryDesc, &Query);
    if (FAILED(Result))
    {
        D3D11_ERROR("[FD3D11Query]: FAILED to create Query (0x%08X)", static_cast<uint32>(Result));
        return false;
    }

    STAT_ADD(STAT_D3D11_QueryCount, 1);
    return true;
}

void FD3D11Query::Begin(ID3D11DeviceContext* Context)
{
    CHECK(Context != nullptr);
    bIsIssued.Store(0);
    Context->Begin(Query.Get());
}

void FD3D11Query::End(ID3D11DeviceContext* Context)
{
    CHECK(Context != nullptr);
    Context->End(Query.Get());
    bIsIssued.Store(1);
}

bool FD3D11Query::GetData(void* OutData, uint32 DataSize, EQueryResultMode Mode) const
{
    if (!bIsIssued.Load())
    {
        return false;
    }

    ID3D11DeviceContext* Context = GetDevice()->GetD3D11Context();
    UINT Flags = Mode == EQueryResultMode::Wait ? 0 : D3D11_ASYNC_GETDATA_DONOTFLUSH;
    HRESULT Result = Context->GetData(Query.Get(), OutData, DataSize, Flags);
    while (Result == S_FALSE && Mode == EQueryResultMode::Wait)
    {
        FPlatformThreadMisc::Yield();
        Result = Context->GetData(Query.Get(), OutData, DataSize, D3D11_ASYNC_GETDATA_DONOTFLUSH);
    }

    if (FAILED(Result))
    {
        GetDevice()->CheckDeviceRemoved(Result, "Query::GetData");
        D3D11_ERROR("[FD3D11Query]: GetData FAILED (0x%08X)", static_cast<uint32>(Result));
        return false;
    }

    return Result == S_OK;
}

void FD3D11Query::SetDebugName(const String& InName)
{
    D3D11SetDebugName(Query.Get(), InName);
}

void FD3D11Query::GetDebugName(String& OutDebugName) const
{
    D3D11GetDebugName(Query.Get(), OutDebugName);
}

FD3D11QueryRHI::FD3D11QueryRHI(FD3D11Device* InDevice, EQueryType InQueryType)
    : FRHIQuery(InQueryType)
    , FD3D11DeviceChild(InDevice)
    , Query(nullptr)
    , DisjointQuery(nullptr)
    , QueryResult(static_cast<uint64*>(Memory::Malloc(GetQueryResultElementCount(InQueryType) * sizeof(uint64))))
    , bResultReady(0)
{
    Memory::Memzero(QueryResult, GetQueryResultElementCount(InQueryType) * sizeof(uint64));
}

FD3D11QueryRHI::~FD3D11QueryRHI()
{
    Memory::Free(QueryResult);
    QueryResult = nullptr;
}

bool FD3D11QueryRHI::Initialize()
{
    FD3D11QueryRef NewQuery = new FD3D11Query(GetDevice(), ConvertQueryType(GetType()));
    if (!NewQuery->Initialize())
    {
        return false;
    }

    NewQuery->SetDebugName(String::Printf("%s Query", ToString(GetType())));
    Query = NewQuery;
    return true;
}

bool FD3D11QueryRHI::ResolveResult(EQueryResultMode Mode)
{
    if (bResultReady.Load())
    {
        return true;
    }

    switch (GetType())
    {
        case EQueryType::Timestamp:
        {
            if (!DisjointQuery)
            {
                return false;
            }

            uint64 Ticks = 0;
            if (!Query->GetData(&Ticks, sizeof(Ticks), Mode))
            {
                return false;
            }

            D3D11_QUERY_DATA_TIMESTAMP_DISJOINT DisjointData = {};
            if (!DisjointQuery->GetData(&DisjointData, sizeof(DisjointData), Mode) || DisjointData.Disjoint)
            {
                return false;
            }

            *QueryResult = ConvertTicksToNanoseconds(Ticks, DisjointData.Frequency);
            break;
        }

        case EQueryType::Occlusion:
        {
            uint64 NumSamples = 0;
            if (!Query->GetData(&NumSamples, sizeof(NumSamples), Mode))
            {
                return false;
            }

            *QueryResult = NumSamples;
            break;
        }

        case EQueryType::PipelineStatistics:
        {
            D3D11_QUERY_DATA_PIPELINE_STATISTICS Src = {};
            if (!Query->GetData(&Src, sizeof(Src), Mode))
            {
                return false;
            }

            FRHIPipelineStatistics* Stats = reinterpret_cast<FRHIPipelineStatistics*>(QueryResult);
            Stats->IAVertices    = Src.IAVertices;
            Stats->IAPrimitives  = Src.IAPrimitives;
            Stats->VSInvocations = Src.VSInvocations;
            Stats->GSInvocations = Src.GSInvocations;
            Stats->GSPrimitives  = Src.GSPrimitives;
            Stats->CInvocations  = Src.CInvocations;
            Stats->CPrimitives   = Src.CPrimitives;
            Stats->PSInvocations = Src.PSInvocations;
            Stats->HSInvocations = Src.HSInvocations;
            Stats->DSInvocations = Src.DSInvocations;
            Stats->CSInvocations = Src.CSInvocations;
            Stats->ASInvocations = 0;
            Stats->MSInvocations = 0;
            Stats->MSPrimitives  = 0;
            break;
        }

        default:
        {
            return false;
        }
    }

    bResultReady.Store(1);
    return true;
}
