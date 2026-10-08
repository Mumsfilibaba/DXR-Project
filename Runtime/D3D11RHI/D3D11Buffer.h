#pragma once
#include "RHI/RHIResources.h"
#include "D3D11RHI/D3D11DeviceChild.h"
#include "D3D11RHI/D3D11Resource.h"

typedef TSharedRef<class FD3D11BufferRHI> FD3D11BufferRHIRef;

class FD3D11BufferRHI : public FRHIBuffer, public FD3D11Resource, public FD3D11DeviceChild
{
public:
    FD3D11BufferRHI(FD3D11Device* InDevice, const FRHIBufferDesc& InBufferDesc);
    virtual ~FD3D11BufferRHI();

    // FRHIBuffer Interface
    virtual void* GetRHINativeResource() const override final { return Resource.Get(); }
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

    virtual void* Map(uint64 Offset = 0, uint64 Size = UINT64_MAX) override final;
    virtual void  Unmap(uint64 Offset = 0, uint64 Size = UINT64_MAX) override final;

    virtual void SetDebugName(const String& InName) override final;
    virtual void GetDebugName(String& OutDebugName) const override final;

    bool Initialize(ERHIResourceState InInitialState, const void* InInitialData);

    FORCEINLINE ID3D11Buffer* GetD3D11Buffer() const
    {
        return static_cast<ID3D11Buffer*>(Resource.Get());
    }
};
