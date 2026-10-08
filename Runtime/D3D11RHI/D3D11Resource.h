#pragma once
#include "RHI/RHITypes.h"
#include "D3D11RHI/D3D11Core.h"

class FD3D11Resource
{
public:
    FD3D11Resource() = default;
    virtual ~FD3D11Resource();

    FORCEINLINE ID3D11Resource* GetD3D11Resource() const
    {
        return Resource.Get();
    }

    FORCEINLINE ERHIResourceState GetCurrentState() const
    {
        return CurrentState;
    }

    FORCEINLINE void SetCurrentState(ERHIResourceState InState)
    {
        CurrentState = InState;
    }

    FORCEINLINE uint64 GetAllocationSize() const
    {
        return AllocationSize;
    }

protected:
    void SetAllocation(D3D11_USAGE InUsage, uint64 InAllocationSize);

    TComPtr<ID3D11Resource> Resource;
    ERHIResourceState       CurrentState   = ERHIResourceState::Common;
    D3D11_USAGE             Usage          = D3D11_USAGE_DEFAULT;
    uint64                  AllocationSize = 0;
};
