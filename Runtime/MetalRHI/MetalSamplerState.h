#pragma once
#include "MetalObject.h"
#include "MetalRefCounted.h"
#include "RHI/RHIResources.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

typedef TSharedRef<class FMetalSamplerState> FMetalSamplerStateRef;

class FMetalSamplerState : public FRHISamplerState, public FMetalObject, public FMetalRefCounted
{
public:
    FMetalSamplerState(FMetalDeviceContext* InDeviceContext, const FRHISamplerStateDesc& InDesc);
    ~FMetalSamplerState();

    bool Initialize();

    virtual int32 AddRef() override final { return FMetalRefCounted::AddRef(); }
    
    virtual int32 Release() override final { return FMetalRefCounted::Release(); }
    
    virtual int32 GetRefCount() const override final { return FMetalRefCounted::GetRefCount(); }

    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }

    id<MTLSamplerState> GetMTLSamplerState() const
    {
        return SamplerState;
    }
    
private:
    id<MTLSamplerState> SamplerState;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
