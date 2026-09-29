#pragma once
#include "RHI/RHITypes.h"
#include "D3D11RHI/D3D11Core.h"

class FD3D11Resource
{
public:
    FD3D11Resource() = default;
    virtual ~FD3D11Resource() = default;

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

protected:
    TComPtr<ID3D11Resource> Resource;
    ERHIResourceState       CurrentState = ERHIResourceState::Common;
};
