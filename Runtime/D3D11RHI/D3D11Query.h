#pragma once
#include "Core/RefCountedBase.h"
#include "Core/Threading/Atomic/AtomicInt.h"
#include "RHI/RHIResources.h"
#include "D3D11RHI/D3D11DeviceChild.h"

typedef TSharedRef<class FD3D11Query>     FD3D11QueryRef;
typedef TSharedRef<struct FD3D11QueryRHI> FD3D11QueryRHIRef;

class FD3D11Query : public FD3D11DeviceChild, public FRefCountedBase
{
public:
    FD3D11Query(FD3D11Device* InDevice, D3D11_QUERY InQueryType);
    ~FD3D11Query();

    bool Initialize();

    void Begin(ID3D11DeviceContext* Context);
    void End(ID3D11DeviceContext* Context);

    bool GetData(void* OutData, uint32 DataSize, EQueryResultMode Mode) const;

    void SetDebugName(const String& InName);
    void GetDebugName(String& OutDebugName) const;

    ID3D11Query* GetD3D11Query() const
    {
        return Query.Get();
    }

private:
    TComPtr<ID3D11Query> Query;
    D3D11_QUERY          QueryType;
    AtomicInt32          bIsIssued;
};

struct FD3D11QueryRHI : public FRHIQuery, public FD3D11DeviceChild
{
    FD3D11QueryRHI(FD3D11Device* InDevice, EQueryType InQueryType);
    virtual ~FD3D11QueryRHI();

    bool Initialize();
    bool ResolveResult(EQueryResultMode Mode);

    FD3D11QueryRef Query;
    FD3D11QueryRef DisjointQuery;
    uint64*        QueryResult;
    AtomicInt32    bResultReady;
};
